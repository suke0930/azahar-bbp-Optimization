// Copyright Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cctype>
#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <json.hpp>
#include "core/3ds.h"
#include "core/core.h"
#include "core/remote/remote_handler.h"

namespace Remote {
namespace {

using Button = Settings::NativeButton::Values;

constexpr int DefaultTapDurationMs = 100;
constexpr int MaxTapDurationMs = 5000;

std::string NormalizeName(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return name;
}

std::optional<Button> ParseButton(std::string name) {
    name = NormalizeName(std::move(name));
    if (name == "a") return Button::A;
    if (name == "b") return Button::B;
    if (name == "x") return Button::X;
    if (name == "y") return Button::Y;
    if (name == "up") return Button::Up;
    if (name == "down") return Button::Down;
    if (name == "left") return Button::Left;
    if (name == "right") return Button::Right;
    if (name == "l") return Button::L;
    if (name == "r") return Button::R;
    if (name == "start") return Button::Start;
    if (name == "select") return Button::Select;
    return std::nullopt;
}

int GetDurationMs(const nlohmann::json& body) {
    const int duration_ms = body.value("duration_ms", DefaultTapDurationMs);
    if (duration_ms < 0 || duration_ms > MaxTapDurationMs) {
        throw std::invalid_argument("duration_ms must be between 0 and 5000");
    }
    return duration_ms;
}

void AdvanceIfPaused(Core::System& system) {
    if (system.frame_limiter.IsFrameAdvancing()) {
        system.frame_limiter.AdvanceFrame();
    }
}

void SleepForDuration(int duration_ms) {
    if (duration_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    }
}

std::vector<Button> ParseButtons(const nlohmann::json& body) {
    if (!body.contains("buttons") || !body["buttons"].is_array() || body["buttons"].empty()) {
        throw std::invalid_argument("buttons must be a non-empty array");
    }

    std::vector<Button> buttons;
    for (const auto& entry : body["buttons"]) {
        const auto button = ParseButton(entry.get<std::string>());
        if (!button) {
            throw std::invalid_argument("unknown button: " + entry.get<std::string>());
        }
        buttons.push_back(*button);
    }
    return buttons;
}

void PressButtons(Core::System& system, const std::vector<Button>& buttons) {
    system.RemoteInput().PressButtons(buttons);
}

void ReleaseButtons(Core::System& system, const std::vector<Button>& buttons) {
    system.RemoteInput().ReleaseButtons(buttons);
}

u16 GetTouchCoordinate(const nlohmann::json& body, const char* key, int max_value) {
    if (!body.contains(key)) {
        throw std::invalid_argument(std::string(key) + " is required");
    }

    const int value = body.at(key).get<int>();
    if (value < 0 || value > max_value) {
        throw std::invalid_argument(std::string(key) + " is out of range");
    }
    return static_cast<u16>(value);
}

} // namespace

nlohmann::json HandleInputButtons(Core::System& system, const nlohmann::json& body) {
    if (!system.IsPoweredOn()) {
        return MakeErrorResponse(400, "Emulator is not running", "not_powered_on");
    }

    const std::string action = NormalizeName(body.value("action", "tap"));
    const auto buttons = ParseButtons(body);

    if (action == "press") {
        PressButtons(system, buttons);
        AdvanceIfPaused(system);
    } else if (action == "release") {
        ReleaseButtons(system, buttons);
        AdvanceIfPaused(system);
    } else if (action == "tap") {
        PressButtons(system, buttons);
        AdvanceIfPaused(system);
        SleepForDuration(GetDurationMs(body));
        ReleaseButtons(system, buttons);
        AdvanceIfPaused(system);
    } else {
        return MakeErrorResponse(400, "unknown input action: " + action, "invalid_action");
    }

    return {{"status", "ok"}};
}

nlohmann::json HandleInputTouch(Core::System& system, const nlohmann::json& body) {
    if (!system.IsPoweredOn()) {
        return MakeErrorResponse(400, "Emulator is not running", "not_powered_on");
    }

    const std::string action = NormalizeName(body.value("action", "tap"));

    if (action == "release") {
        system.RemoteInput().ReleaseTouch();
        AdvanceIfPaused(system);
        return {{"status", "ok"}};
    }

    const u16 x = GetTouchCoordinate(body, "x", Core::kScreenBottomWidth - 1);
    const u16 y = GetTouchCoordinate(body, "y", Core::kScreenBottomHeight - 1);

    if (action == "press") {
        system.RemoteInput().PressTouch(x, y);
        AdvanceIfPaused(system);
    } else if (action == "move") {
        system.RemoteInput().MoveTouch(x, y);
        AdvanceIfPaused(system);
    } else if (action == "tap") {
        system.RemoteInput().PressTouch(x, y);
        AdvanceIfPaused(system);
        SleepForDuration(GetDurationMs(body));
        system.RemoteInput().ReleaseTouch();
        AdvanceIfPaused(system);
    } else {
        return MakeErrorResponse(400, "unknown input action: " + action, "invalid_action");
    }

    return {{"status", "ok"}};
}

nlohmann::json HandleInputReleaseAll(Core::System& system, const nlohmann::json& /*body*/) {
    if (!system.IsPoweredOn()) {
        return MakeErrorResponse(400, "Emulator is not running", "not_powered_on");
    }

    system.RemoteInput().ReleaseAll();
    AdvanceIfPaused(system);
    return {{"status", "ok"}};
}

} // namespace Remote
