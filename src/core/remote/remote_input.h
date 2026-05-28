// Copyright Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <atomic>
#include <cstddef>

#include "common/common_types.h"
#include "common/settings.h"

namespace Remote {

class InputState {
public:
    struct TouchStatus {
        u16 x = 0;
        u16 y = 0;
        bool pressed = false;
    };

    void PressButton(Settings::NativeButton::Values button);
    void ReleaseButton(Settings::NativeButton::Values button);
    void ReleaseAll();

    [[nodiscard]] bool GetButtonStatus(Settings::NativeButton::Values button) const;

    void PressTouch(u16 x, u16 y);
    void MoveTouch(u16 x, u16 y);
    void ReleaseTouch();
    [[nodiscard]] TouchStatus GetTouchStatus() const;

private:
    [[nodiscard]] static bool IsHidButton(Settings::NativeButton::Values button);
    [[nodiscard]] static std::size_t ButtonIndex(Settings::NativeButton::Values button);

    std::array<std::atomic_bool, Settings::NativeButton::NUM_BUTTONS_HID> buttons{};
    std::atomic<u16> touch_x{0};
    std::atomic<u16> touch_y{0};
    std::atomic_bool touch_pressed{false};
};

} // namespace Remote
