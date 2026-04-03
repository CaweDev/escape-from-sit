/**
 * @file Input.cpp
 * @brief Implementation of Input class
 */
#include "Input.hpp"
#include "XboxController.hpp"

namespace Engine {

// Static member definitions
std::set<std::string> Input::cursorUnlockRegistrations;
HWND Input::hwnd = nullptr;
int Input::centerX = 0;
int Input::centerY = 0;
f32 Input::mouseDeltaX = 0.0f;
f32 Input::mouseDeltaY = 0.0f;
bool Input::firstFrame = true;



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
    if (IsCursorLocked()) {
        // Return the delta calculated from centered cursor
        return Vector2(mouseDeltaX, mouseDeltaY);
    }
    // Fallback to Alpha Engine method when not locked
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
        
        // Add Xbox controller left stick X
        if (XboxController::IsConnected(0)) {
            value += XboxController::GetLeftStick(0).x;
            // Clamp to -1 to 1
            if (value < -1.0f) value = -1.0f;
            if (value > 1.0f) value = 1.0f;
        }
        
        return value;
    }
    else if (axisName == "Vertical") {
        f32 value = 0;
        if (GetKey(KeyCode::S) || GetKey(KeyCode::DownArrow)) value -= 1;
        if (GetKey(KeyCode::W) || GetKey(KeyCode::UpArrow)) value += 1;
        
        // Add Xbox controller left stick Y
        if (XboxController::IsConnected(0)) {
            value += XboxController::GetLeftStick(0).y;
            // Clamp to -1 to 1
            if (value < -1.0f) value = -1.0f;
            if (value > 1.0f) value = 1.0f;
        }
        
        return value;
    }
    else if (axisName == "Mouse X") {
        f32 value = GetMouseDelta().x;
        
        // Add Xbox controller right stick X
        if (XboxController::IsConnected(0)) {
            value += XboxController::GetRightStick(0).x * 300.0f; // Scale for camera movement
        }
        
        return value;
    }
    else if (axisName == "Mouse Y") {
        f32 value = GetMouseDelta().y;
        
        // Add Xbox controller right stick Y
        if (XboxController::IsConnected(0)) {
            value += XboxController::GetRightStick(0).y * 300.0f; // Scale for camera movement
        }
        
        return value;
    }
    return 0;
}



f32 Input::GetAxisRaw(const std::string& axisName) {
    return GetAxis(axisName);
}



void Input::SetCursorVisible(bool visible) {
    AEInputShowCursor(visible ? 1 : 0);
}



void Input::RegisterCursorUnlock(const std::string& id) {
    bool wasEmpty = cursorUnlockRegistrations.empty();
    cursorUnlockRegistrations.insert(id);
    
    if (wasEmpty && !cursorUnlockRegistrations.empty()) {
        // First registration - unlock cursor
        SetCursorVisible(true);
    }
}



void Input::UnregisterCursorUnlock(const std::string& id) {
    cursorUnlockRegistrations.erase(id);
    
    if (cursorUnlockRegistrations.empty()) {
        // All unregistered - lock cursor
        SetCursorVisible(false);
        firstFrame = true; // Will trigger lock update next frame
    }
}



bool Input::IsCursorLocked() {
    return cursorUnlockRegistrations.empty();
}


void Input::Update() {
    // Update Xbox controller state first
    XboxController::Update();
    
    // Reset delta each frame
    mouseDeltaX = 0.0f;
    mouseDeltaY = 0.0f;
    
    if (!IsCursorLocked()) {
        return;
    }
    
    // Skip first frame to avoid spinning on initial lock and apply lock state
    if (firstFrame) {
        firstFrame = false;
        ShowCursor(FALSE);
        hwnd = GetActiveWindow();
        UpdateWindowCenter();
        CenterCursor();
        return;
    }
    
    // Get current cursor position
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    
    // Calculate delta from center
    mouseDeltaX = static_cast<f32>(cursorPos.x - centerX);
    mouseDeltaY = static_cast<f32>(cursorPos.y - centerY);
    
    // Re-center cursor
    CenterCursor();
}


void Input::UpdateWindowCenter() {
    if (!hwnd) hwnd = GetActiveWindow();
    if (hwnd) {
        RECT rect;
        GetWindowRect(hwnd, &rect);
        centerX = (rect.left + rect.right) / 2;
        centerY = (rect.top + rect.bottom) / 2;
    }
}


void Input::CenterCursor() {
    SetCursorPos(centerX, centerY);
}

} // namespace Engine
