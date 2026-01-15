/**
 * @file Input.hpp
 * @brief Static input class with Unity-like API
 */
#pragma once

#include "KeyCode.hpp"
#include "../Math/Vector2.hpp"
#include "AEEngine.h"
#include <string>

namespace Engine {

/**
 * @class Input
 * @brief Static class for querying input state
 */
class Input {
public:
    // Keyboard input
    
    /** Check if key is currently held down */
    static bool GetKey(KeyCode key);
    
    /** Check if key was pressed this frame */
    static bool GetKeyDown(KeyCode key);
    
    /** Check if key was released this frame */
    static bool GetKeyUp(KeyCode key);
    
    // Mouse input
    
    /** Check if mouse button is currently held (0=left, 1=right, 2=middle) */
    static bool GetMouseButton(int button);
    
    /** Check if mouse button was pressed this frame */
    static bool GetMouseButtonDown(int button);
    
    /** Check if mouse button was released this frame */
    static bool GetMouseButtonUp(int button);
    
    /** Get current mouse position in screen coordinates */
    static Vector2 GetMousePosition();
    
    /** Get mouse movement delta since last frame */
    static Vector2 GetMouseDelta();
    
    /** Get mouse scroll wheel delta */
    static f32 GetMouseScrollDelta();
    
    // Axis input (Unity-style)
    
    /** Get axis value (-1 to 1) for named axis */
    static f32 GetAxis(const std::string& axisName);
    
    /** Get raw axis value (no smoothing, same as GetAxis for now) */
    static f32 GetAxisRaw(const std::string& axisName);
    
    // Cursor control
    
    /** Show or hide the cursor */
    static void SetCursorVisible(bool visible);

private:
    Input() = delete; // Static class
};

} // namespace Engine
