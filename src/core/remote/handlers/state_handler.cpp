// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>

#include <json.hpp>
#include "core/core.h"
#include "core/remote/remote_handler.h"

namespace Remote {

nlohmann::json HandleStateSave(Core::System& system, const nlohmann::json& body) {
    const int slot = body.value("slot", 0);
    const int clamped = std::clamp(slot, 0, 10);
    if (!system.SendSignal(Core::System::Signal::Save, clamped)) {
        return MakeErrorResponse(409, "Signal already pending", "signal_pending");
    }
    return {{"status", "ok"}, {"slot", clamped}};
}

nlohmann::json HandleStateLoad(Core::System& system, const nlohmann::json& body) {
    const int slot = body.value("slot", 0);
    const int clamped = std::clamp(slot, 0, 10);
    if (!system.SendSignal(Core::System::Signal::Load, clamped)) {
        return MakeErrorResponse(409, "Signal already pending", "signal_pending");
    }
    return {{"status", "ok"}, {"slot", clamped}};
}

nlohmann::json HandleStateList(Core::System& /*system*/, const nlohmann::json& /*body*/) {
    return {{"states", nlohmann::json::array()}};
}

} // namespace Remote
