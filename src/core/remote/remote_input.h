// Copyright Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <cstddef>
#include <mutex>
#include <vector>

#include "common/common_types.h"
#include "common/settings.h"

namespace Remote {

class InputState {
public:
    using ButtonSnapshot = std::array<bool, Settings::NativeButton::NUM_BUTTONS_HID>;

    struct TouchStatus {
        u16 x = 0;
        u16 y = 0;
        bool pressed = false;
    };

    void PressButtons(const std::vector<Settings::NativeButton::Values>& buttons);
    void ReleaseButtons(const std::vector<Settings::NativeButton::Values>& buttons);
    void ReleaseAll();

    [[nodiscard]] ButtonSnapshot GetButtonSnapshot() const;

    void PressTouch(u16 x, u16 y);
    void MoveTouch(u16 x, u16 y);
    void ReleaseTouch();
    [[nodiscard]] TouchStatus GetTouchStatus() const;

private:
    [[nodiscard]] static bool IsHidButton(Settings::NativeButton::Values button);
    [[nodiscard]] static std::size_t ButtonIndex(Settings::NativeButton::Values button);
    void PressButtonLocked(Settings::NativeButton::Values button);
    void ReleaseButtonLocked(Settings::NativeButton::Values button);

    mutable std::mutex mutex;
    ButtonSnapshot buttons{};
    TouchStatus touch{};
};

} // namespace Remote
