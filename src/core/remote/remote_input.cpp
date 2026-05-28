// Copyright Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/remote/remote_input.h"

namespace Remote {

bool InputState::IsHidButton(Settings::NativeButton::Values button) {
    return button >= Settings::NativeButton::BUTTON_HID_BEGIN &&
           button < Settings::NativeButton::BUTTON_HID_END;
}

std::size_t InputState::ButtonIndex(Settings::NativeButton::Values button) {
    return static_cast<std::size_t>(button - Settings::NativeButton::BUTTON_HID_BEGIN);
}

void InputState::PressButton(Settings::NativeButton::Values button) {
    if (IsHidButton(button)) {
        buttons[ButtonIndex(button)].store(true);
    }
}

void InputState::ReleaseButton(Settings::NativeButton::Values button) {
    if (IsHidButton(button)) {
        buttons[ButtonIndex(button)].store(false);
    }
}

void InputState::ReleaseAll() {
    for (auto& button : buttons) {
        button.store(false);
    }
    ReleaseTouch();
}

bool InputState::GetButtonStatus(Settings::NativeButton::Values button) const {
    if (!IsHidButton(button)) {
        return false;
    }
    return buttons[ButtonIndex(button)].load();
}

void InputState::PressTouch(u16 x, u16 y) {
    touch_x.store(x);
    touch_y.store(y);
    touch_pressed.store(true);
}

void InputState::MoveTouch(u16 x, u16 y) {
    touch_x.store(x);
    touch_y.store(y);
    touch_pressed.store(true);
}

void InputState::ReleaseTouch() {
    touch_pressed.store(false);
    touch_x.store(0);
    touch_y.store(0);
}

InputState::TouchStatus InputState::GetTouchStatus() const {
    return {
        .x = touch_x.load(),
        .y = touch_y.load(),
        .pressed = touch_pressed.load(),
    };
}

} // namespace Remote
