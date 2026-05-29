// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <chrono>

#include <json.hpp>
#include "core/core.h"
#include "core/savestate.h"
#include "core/remote/remote_handler.h"

namespace Remote {
namespace {

constexpr auto StateOperationTimeout = std::chrono::seconds{10};

nlohmann::json MakeEmptySaveStateEntry(u32 slot) {
    return {{"slot", slot},
            {"exists", false},
            {"time", nullptr},
            {"build_name", nullptr},
            {"status", nullptr}};
}

void PopulateSaveStateEntry(nlohmann::json& entry, const Core::SaveStateInfo& info) {
    entry["exists"] = true;
    entry["time"] = info.time;
    entry["build_name"] = info.build_name;

    switch (info.status) {
    case Core::SaveStateInfo::ValidationStatus::OK:
        entry["status"] = "ok";
        break;
    case Core::SaveStateInfo::ValidationStatus::RevisionDismatch:
        entry["status"] = "revision_mismatch";
        break;
    }
}

nlohmann::json HandleStateOperation(Core::System& system, Core::System::Signal signal, int slot) {
    const auto result =
        system.RequestStateOperation(signal, static_cast<u32>(slot), StateOperationTimeout);

    switch (result.operation_status) {
    case Core::System::StateOperationStatus::Completed:
        if (result.result_status == Core::System::ResultStatus::Success) {
            return {{"status", "ok"}, {"slot", slot}};
        }
        return MakeErrorResponse(500,
                                 result.details.empty() ? "State operation failed" : result.details,
                                 "state_operation_failed");
    case Core::System::StateOperationStatus::SignalPending:
        return MakeErrorResponse(409, "Signal already pending", "signal_pending");
    case Core::System::StateOperationStatus::TimedOut:
        return MakeErrorResponse(504, "Timed out waiting for state operation",
                                 "state_operation_timeout");
    case Core::System::StateOperationStatus::InvalidSignal:
        return MakeErrorResponse(400, "Invalid state operation", "invalid_state_operation");
    }

    return MakeErrorResponse(500, "State operation failed", "state_operation_failed");
}

} // namespace

nlohmann::json BuildStateListResponse(const std::vector<Core::SaveStateInfo>& savestates) {
    nlohmann::json states = nlohmann::json::array();
    for (u32 slot = 0; slot <= Core::SaveStateSlotCount; ++slot) {
        states.push_back(MakeEmptySaveStateEntry(slot));
    }

    for (const auto& savestate : savestates) {
        if (savestate.slot > Core::SaveStateSlotCount) {
            continue;
        }
        PopulateSaveStateEntry(states[savestate.slot], savestate);
    }

    return {{"status", "ok"}, {"states", std::move(states)}};
}

nlohmann::json HandleStateSave(Core::System& system, const nlohmann::json& body) {
    if (!system.IsPoweredOn()) {
        return MakeErrorResponse(400, "Emulator is not running", "not_powered_on");
    }
    const int slot = body.value("slot", 0);
    const int clamped_slot = std::clamp(slot, 0, 10);
    return HandleStateOperation(system, Core::System::Signal::Save, clamped_slot);
}

nlohmann::json HandleStateLoad(Core::System& system, const nlohmann::json& body) {
    if (!system.IsPoweredOn()) {
        return MakeErrorResponse(400, "Emulator is not running", "not_powered_on");
    }
    const int slot = body.value("slot", 0);
    const int clamped_slot = std::clamp(slot, 0, 10);
    return HandleStateOperation(system, Core::System::Signal::Load, clamped_slot);
}

nlohmann::json HandleStateList(Core::System& system, const nlohmann::json& /*body*/) {
    const u64 title_id = system.GetTitleId();
    if (!system.IsPoweredOn() || title_id == 0) {
        return MakeErrorResponse(400, "Emulator is not running", "not_powered_on");
    }

    const auto savestates = Core::ListSaveStates(title_id, system.Movie().GetCurrentMovieID());

    return BuildStateListResponse(savestates);
}

} // namespace Remote
