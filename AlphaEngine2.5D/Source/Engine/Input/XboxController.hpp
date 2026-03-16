/**
 * @file XboxController.hpp
 * @brief Xbox controller support using XInput API
 */
#pragma once

#include <Windows.h>
#include <Xinput.h>
#include "../Math/Vector2.hpp"

#pragma comment(lib, "XInput.lib")

namespace Engine {

/**
 * @enum XboxButton
 * @brief Xbox controller button codes
 */
enum class XboxButton {
    DPadUp = XINPUT_GAMEPAD_DPAD_UP,
    DPadDown = XINPUT_GAMEPAD_DPAD_DOWN,
    DPadLeft = XINPUT_GAMEPAD_DPAD_LEFT,
    DPadRight = XINPUT_GAMEPAD_DPAD_RIGHT,
    Start = XINPUT_GAMEPAD_START,
    Back = XINPUT_GAMEPAD_BACK,
    LeftThumb = XINPUT_GAMEPAD_LEFT_THUMB,
    RightThumb = XINPUT_GAMEPAD_RIGHT_THUMB,
    LeftShoulder = XINPUT_GAMEPAD_LEFT_SHOULDER,
    RightShoulder = XINPUT_GAMEPAD_RIGHT_SHOULDER,
    A = XINPUT_GAMEPAD_A,
    B = XINPUT_GAMEPAD_B,
    X = XINPUT_GAMEPAD_X,
    Y = XINPUT_GAMEPAD_Y
};

/**
 * @class XboxController
 * @brief Static class for querying Xbox controller state
 */
class XboxController {
public:
    /** Check if a specific controller is connected (0-3) */
    static bool IsConnected(int controllerIndex = 0);
    
    /** Check if button is currently held */
    static bool GetButton(XboxButton button, int controllerIndex = 0);
    
    /** Check if button was pressed this frame */
    static bool GetButtonDown(XboxButton button, int controllerIndex = 0);
    
    /** Check if button was released this frame */
    static bool GetButtonUp(XboxButton button, int controllerIndex = 0);
    
    /** Get left stick position (-1 to 1 for both axes) */
    static Vector2 GetLeftStick(int controllerIndex = 0);
    
    /** Get right stick position (-1 to 1 for both axes) */
    static Vector2 GetRightStick(int controllerIndex = 0);
    
    /** Get left trigger value (0 to 1) */
    static f32 GetLeftTrigger(int controllerIndex = 0);
    
    /** Get right trigger value (0 to 1) */
    static f32 GetRightTrigger(int controllerIndex = 0);
    
    /** Set vibration (0 to 1 for both motors) */
    static void SetVibration(f32 leftMotor, f32 rightMotor, int controllerIndex = 0);
    
    /** Must be called each frame to update controller state */
    static void Update();
    
    /** Get any connected controller index (-1 if none) */
    static int GetFirstConnectedController();

private:
    XboxController() = delete; // Static class
    
    static XINPUT_STATE currentState[XUSER_MAX_COUNT];
    static XINPUT_STATE previousState[XUSER_MAX_COUNT];
    static bool connected[XUSER_MAX_COUNT];
    
    static f32 ApplyDeadzone(SHORT value, SHORT deadzone);
    static bool IsButtonPressed(const XINPUT_STATE& state, XboxButton button);
};

} // namespace Engine
