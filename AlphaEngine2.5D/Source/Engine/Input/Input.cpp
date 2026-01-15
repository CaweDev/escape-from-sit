/**
 * @file Input.cpp
 * @brief Implementation of Input class
 */
#include "Input.hpp"

namespace Engine {

bool Input::GetKey(KeyCode key) {
    return AEInputCheckCurr(static_cast<u8>(key)) != 0;
}

bool Input::GetKeyDown(KeyCode key) {
    return AEInputCheckTriggered(static_cast<u8>(key)) != 0;
}

bool Input::GetKeyUp(KeyCode key) {
    return AEInputCheckReleased(static_cast<u8>(key)) != 0;
}

bool Input::GetMouseButton(int button) {
    u8 key;
    switch (button) {
        case 0: key = AEVK_LBUTTON; break;
        case 1: key = AEVK_RBUTTON; break;
        case 2: key = AEVK_MBUTTON; break;
        default: return false;
    }
    return AEInputCheckCurr(key) != 0;
}

bool Input::GetMouseButtonDown(int button) {
    u8 key;
    switch (button) {
        case 0: key = AEVK_LBUTTON; break;
        case 1: key = AEVK_RBUTTON; break;
        case 2: key = AEVK_MBUTTON; break;
        default: return false;
    }
    return AEInputCheckTriggered(key) != 0;
}

bool Input::GetMouseButtonUp(int button) {
    u8 key;
    switch (button) {
        case 0: key = AEVK_LBUTTON; break;
        case 1: key = AEVK_RBUTTON; break;
        case 2: key = AEVK_MBUTTON; break;
        default: return false;
    }
    return AEInputCheckReleased(key) != 0;
}

Vector2 Input::GetMousePosition() {
    s32 x, y;
    AEInputGetCursorPosition(&x, &y);
    return Vector2(static_cast<f32>(x), static_cast<f32>(y));
}

Vector2 Input::GetMouseDelta() {
    s32 dx, dy;
    AEInputGetCursorPositionDelta(&dx, &dy);
    return Vector2(static_cast<f32>(dx), static_cast<f32>(dy));
}

f32 Input::GetMouseScrollDelta() {
    s32 delta;
    AEInputMouseWheelDelta(&delta);
    return static_cast<f32>(delta);
}

f32 Input::GetAxis(const std::string& axisName) {
    if (axisName == "Horizontal") {
        f32 value = 0;
        if (GetKey(KeyCode::A) || GetKey(KeyCode::LeftArrow)) value -= 1;
        if (GetKey(KeyCode::D) || GetKey(KeyCode::RightArrow)) value += 1;
        return value;
    }
    else if (axisName == "Vertical") {
        f32 value = 0;
        if (GetKey(KeyCode::S) || GetKey(KeyCode::DownArrow)) value -= 1;
        if (GetKey(KeyCode::W) || GetKey(KeyCode::UpArrow)) value += 1;
        return value;
    }
    else if (axisName == "Mouse X") {
        return GetMouseDelta().x;
    }
    else if (axisName == "Mouse Y") {
        return GetMouseDelta().y;
    }
    return 0;
}

f32 Input::GetAxisRaw(const std::string& axisName) {
    return GetAxis(axisName);
}

void Input::SetCursorVisible(bool visible) {
    AEInputShowCursor(visible ? 1 : 0);
}

} // namespace Engine
