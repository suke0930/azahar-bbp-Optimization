// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>

#include <fmt/format.h>
#include <json.hpp>
#include "common/settings.h"
#include "core/core.h"

namespace Remote {

namespace {

nlohmann::json HandleEmulatorControl(Core::System& system, const nlohmann::json& body) {
    const std::string action = body.value("action", "");

    if (action == "pause") {
        system.frame_limiter.SetFrameAdvancing(true);
        return {{"status", "ok"}, {"state", "paused"}};
    } else if (action == "resume") {
        system.frame_limiter.SetFrameAdvancing(false);
        system.frame_limiter.AdvanceFrame();
        return {{"status", "ok"}, {"state", "running"}};
    } else if (action == "stop") {
        system.SendSignal(Core::System::Signal::Shutdown);
        return {{"status", "ok"}, {"state", "stopped"}};
    } else if (action == "reset") {
        system.SendSignal(Core::System::Signal::Reset);
        return {{"status", "ok"}, {"state", "running"}};
    } else {
        return {{"status", "error"}, {"message", "unknown action"}};
    }
}

nlohmann::json HandleEmulatorSpeed(Core::System& system, const nlohmann::json& body) {
    const int speed_percent = body.value("speed_percent", 100);
    const int clamped = std::clamp(speed_percent, 0, 1000);
    Settings::values.frame_limit.SetValue(static_cast<double>(clamped));
    return {{"status", "ok"}, {"current_speed", clamped}};
}

nlohmann::json HandleEmulatorStatus(Core::System& system, const nlohmann::json& /*body*/) {
    const bool is_powered_on = system.IsPoweredOn();
    std::string state = "stopped";
    if (is_powered_on) {
        state = system.frame_limiter.IsFrameAdvancing() ? "paused" : "running";
    }
    return {
        {"state", state},
        {"is_powered_on", is_powered_on},
        {"title_id", fmt::format("{:016X}", system.GetTitleId())},
    };
}

} // namespace
} // namespace Remote
