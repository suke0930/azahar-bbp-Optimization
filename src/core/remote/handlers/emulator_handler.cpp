// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>

#include <fmt/format.h>
#include <json.hpp>
#include "common/settings.h"
#include "core/core.h"

namespace Remote {

nlohmann::json HandleEmulatorControl(Core::System& system, const nlohmann::json& body) {
    const std::string action = body.value("action", "");

    if (action == "pause") {
        system.frame_limiter.SetFrameAdvancing(true);
        return {{"status", "ok"}, {"state", "paused"}};
    } else if (action == "resume") {
        system.frame_limiter.SetFrameAdvancing(false);
        return {{"status", "ok"}, {"state", "running"}};
    } else if (action == "stop") {
        if (!system.SendSignal(Core::System::Signal::Shutdown)) {
            return {{"status", "error"}, {"message", "signal already pending"}};
        }
        return {{"status", "ok"}, {"state", "stopped"}};
    } else if (action == "reset") {
        if (!system.SendSignal(Core::System::Signal::Reset)) {
            return {{"status", "error"}, {"message", "signal already pending"}};
        }
        return {{"status", "ok"}, {"state", "running"}};
    } else {
        throw std::invalid_argument("unknown action: " + action);
    }
}

nlohmann::json HandleEmulatorSpeed(Core::System& system, const nlohmann::json& body) {
    const int speed_percent = body.value("speed_percent", 100);
    const int clamped = std::clamp(speed_percent, 0, 1000);
    Settings::values.frame_limit.SetValue(static_cast<double>(clamped));
    system.ApplySettings();
    return {{"status", "ok"}, {"current_speed", clamped}};
}

nlohmann::json HandleEmulatorStatus(Core::System& system, const nlohmann::json& /*body*/) {
    const bool is_powered_on = system.IsPoweredOn();
    std::string state = "stopped";
    std::string title_id = "0000000000000000";
    if (is_powered_on) {
        state = system.frame_limiter.IsFrameAdvancing() ? "paused" : "running";
        title_id = fmt::format("{:016X}", system.GetTitleId());
    }
    return {
        {"state", state},
        {"is_powered_on", is_powered_on},
        {"title_id", title_id},
    };
}

} // namespace Remote
