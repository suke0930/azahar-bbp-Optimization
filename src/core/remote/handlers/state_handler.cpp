// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>

#include <json.hpp>
#include "core/core.h"
#include "core/remote/remote_handler.h"

namespace Remote {

nlohmann::json HandleStateSave(Core::System& system, const nlohmann::json& body) {
    if (!system.IsPoweredOn()) {
        return MakeErrorResponse(400, "Emulator is not running", "not_powered_on");
    }
    const int slot = body.value("slot", 0);
    const int clamped_slot = std::clamp(slot, 0, 10);
    if (!system.SendSignal(Core::System::Signal::Save, clamped_slot)) {
        return MakeErrorResponse(409, "Signal already pending", "signal_pending");
    }
    return {{"status", "ok"}, {"slot", clamped_slot}};
}

nlohmann::json HandleStateLoad(Core::System& system, const nlohmann::json& body) {
    if (!system.IsPoweredOn()) {
        return MakeErrorResponse(400, "Emulator is not running", "not_powered_on");
    }
    const int slot = body.value("slot", 0);
    const int clamped_slot = std::clamp(slot, 0, 10);
    if (!system.SendSignal(Core::System::Signal::Load, clamped_slot)) {
        return MakeErrorResponse(409, "Signal already pending", "signal_pending");
    }
    return {{"status", "ok"}, {"slot", clamped_slot}};
}

nlohmann::json HandleStateList(Core::System& /*system*/, const nlohmann::json& /*body*/) {
    return {{"status", "not_implemented"}};
}

} // namespace Remote
