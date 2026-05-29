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

void InputState::PressButtonLocked(Settings::NativeButton::Values button) {
    if (IsHidButton(button)) {
        buttons[ButtonIndex(button)] = true;
    }
}

void InputState::ReleaseButtonLocked(Settings::NativeButton::Values button) {
    if (IsHidButton(button)) {
        buttons[ButtonIndex(button)] = false;
    }
}

void InputState::PressButtons(const std::vector<Settings::NativeButton::Values>& button_list) {
    std::scoped_lock lock{mutex};
    for (const auto button : button_list) {
        PressButtonLocked(button);
    }
}

void InputState::ReleaseButtons(const std::vector<Settings::NativeButton::Values>& button_list) {
    std::scoped_lock lock{mutex};
    for (const auto button : button_list) {
        ReleaseButtonLocked(button);
    }
}

void InputState::ReleaseAll() {
    std::scoped_lock lock{mutex};
    buttons.fill(false);
    touch = {};
}

InputState::ButtonSnapshot InputState::GetButtonSnapshot() const {
    std::scoped_lock lock{mutex};
    return buttons;
}

void InputState::PressTouch(u16 x, u16 y) {
    std::scoped_lock lock{mutex};
    touch = {.x = x, .y = y, .pressed = true};
}

void InputState::MoveTouch(u16 x, u16 y) {
    std::scoped_lock lock{mutex};
    touch = {.x = x, .y = y, .pressed = true};
}

void InputState::ReleaseTouch() {
    std::scoped_lock lock{mutex};
    touch = {};
}

InputState::TouchStatus InputState::GetTouchStatus() const {
    std::scoped_lock lock{mutex};
    return touch;
}

} // namespace Remote
