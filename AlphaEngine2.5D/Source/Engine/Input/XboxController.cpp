/**
 * @file XboxController.cpp
 * @brief Implementation of Xbox controller support
 */
#include "XboxController.hpp"
#include <cmath>

namespace Engine {

// Static member definitions
XINPUT_STATE XboxController::currentState[XUSER_MAX_COUNT] = {};
XINPUT_STATE XboxController::previousState[XUSER_MAX_COUNT] = {};
bool XboxController::connected[XUSER_MAX_COUNT] = {};

/*!
 * \brief [AUTO] IsConnected
 * \param controllerIndex [in]
 * \return 
 */
bool XboxController::IsConnected(int controllerIndex) {
    if (controllerIndex < 0 || controllerIndex >= XUSER_MAX_COUNT) return false;
    return connected[controllerIndex];
}

/*!
 * \brief [AUTO] GetButton
 * \param button [in]
 * \param controllerIndex [in]
 * \return 
 */
bool XboxController::GetButton(XboxButton button, int controllerIndex) {
    if (!IsConnected(controllerIndex)) return false;
    return IsButtonPressed(currentState[controllerIndex], button);
}

/*!
 * \brief [AUTO] GetButtonDown
 * \param button [in]
 * \param controllerIndex [in]
 * \return 
 */
bool XboxController::GetButtonDown(XboxButton button, int controllerIndex) {
    if (!IsConnected(controllerIndex)) return false;
    bool current = IsButtonPressed(currentState[controllerIndex], button);
    bool previous = IsButtonPressed(previousState[controllerIndex], button);
    return current && !previous;
}

/*!
 * \brief [AUTO] GetButtonUp
 * \param button [in]
 * \param controllerIndex [in]
 * \return 
 */
bool XboxController::GetButtonUp(XboxButton button, int controllerIndex) {
    if (!IsConnected(controllerIndex)) return false;
    bool current = IsButtonPressed(currentState[controllerIndex], button);
    bool previous = IsButtonPressed(previousState[controllerIndex], button);
    return !current && previous;
}

/*!
 * \brief [AUTO] GetLeftStick
 * \param controllerIndex [in]
 * \return 
 */
Vector2 XboxController::GetLeftStick(int controllerIndex) {
    if (!IsConnected(controllerIndex)) return Vector2::zero();
    
    f32 x = ApplyDeadzone(currentState[controllerIndex].Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    f32 y = ApplyDeadzone(currentState[controllerIndex].Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    
    return Vector2(x, y);
}

/*!
 * \brief [AUTO] GetRightStick
 * \param controllerIndex [in]
 * \return 
 */
Vector2 XboxController::GetRightStick(int controllerIndex) {
    if (!IsConnected(controllerIndex)) return Vector2::zero();
    
    f32 x = ApplyDeadzone(currentState[controllerIndex].Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    f32 y = ApplyDeadzone(currentState[controllerIndex].Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    
    return Vector2(x, y);
}

/*!
 * \brief [AUTO] GetLeftTrigger
 * \param controllerIndex [in]
 * \return 
 */
f32 XboxController::GetLeftTrigger(int controllerIndex) {
    if (!IsConnected(controllerIndex)) return 0.0f;
    
    BYTE value = currentState[controllerIndex].Gamepad.bLeftTrigger;
    if (value < XINPUT_GAMEPAD_TRIGGER_THRESHOLD) return 0.0f;
    
    return static_cast<f32>(value) / 255.0f;
}

/*!
 * \brief [AUTO] GetRightTrigger
 * \param controllerIndex [in]
 * \return 
 */
f32 XboxController::GetRightTrigger(int controllerIndex) {
    if (!IsConnected(controllerIndex)) return 0.0f;
    
    BYTE value = currentState[controllerIndex].Gamepad.bRightTrigger;
    if (value < XINPUT_GAMEPAD_TRIGGER_THRESHOLD) return 0.0f;
    
    return static_cast<f32>(value) / 255.0f;
}

/*!
 * \brief [AUTO] SetVibration
 * \param leftMotor [in]
 * \param rightMotor [in]
 * \param controllerIndex [in]
 */
void XboxController::SetVibration(f32 leftMotor, f32 rightMotor, int controllerIndex) {
    if (!IsConnected(controllerIndex)) return;
    
    XINPUT_VIBRATION vibration;
    vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
    vibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);
    
    XInputSetState(controllerIndex, &vibration);
}

/*!
 * \brief [AUTO] Update
 */
void XboxController::Update() {
    for (int i = 0; i < XUSER_MAX_COUNT; ++i) {
        // Store previous state
        previousState[i] = currentState[i];
        
        // Get new state
        ZeroMemory(&currentState[i], sizeof(XINPUT_STATE));
        DWORD result = XInputGetState(i, &currentState[i]);
        connected[i] = (result == ERROR_SUCCESS);
    }
}

/*!
 * \brief [AUTO] GetFirstConnectedController
 * \return 
 */
int XboxController::GetFirstConnectedController() {
    for (int i = 0; i < XUSER_MAX_COUNT; ++i) {
        if (connected[i]) return i;
    }
    return -1;
}

/*!
 * \brief [AUTO] ApplyDeadzone
 * \param value [in]
 * \param deadzone [in]
 * \return 
 */
f32 XboxController::ApplyDeadzone(SHORT value, SHORT deadzone) {
    if (value < -deadzone) {
        // Negative zone
        f32 normalized = static_cast<f32>(value + deadzone) / (32768.0f - deadzone);
        return normalized;
    }
    else if (value > deadzone) {
        // Positive zone
        f32 normalized = static_cast<f32>(value - deadzone) / (32767.0f - deadzone);
        return normalized;
    }
    else {
        // Dead zone
        return 0.0f;
    }
}

/*!
 * \brief [AUTO] IsButtonPressed
 * \param state [in]
 * \param button [in]
 * \return 
 */
bool XboxController::IsButtonPressed(const XINPUT_STATE& state, XboxButton button) {
    return (state.Gamepad.wButtons & static_cast<WORD>(button)) != 0;
}

} // namespace Engine
