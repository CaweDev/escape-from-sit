/**
 * @file KeyCode.hpp
 * @brief Unity-like key code enum mapping to Alpha Engine codes
 */
#pragma once

#include "AEEngine.h"

namespace Engine {

/**
 * @enum KeyCode
 * @brief Key codes for keyboard and mouse input (Unity-style naming)
 */
enum class KeyCode {
    // Mouse buttons
    Mouse0 = AEVK_LBUTTON,
    Mouse1 = AEVK_RBUTTON,
    Mouse2 = AEVK_MBUTTON,
    
    // Special keys
    Backspace = AEVK_BACK,
    Tab = AEVK_TAB,
    Return = AEVK_RETURN,
    Escape = AEVK_ESCAPE,
    Space = AEVK_SPACE,
    
    // Arrow keys
    UpArrow = AEVK_UP,
    DownArrow = AEVK_DOWN,
    LeftArrow = AEVK_LEFT,
    RightArrow = AEVK_RIGHT,
    
    // Modifiers
    LeftShift = AEVK_LSHIFT,
    RightShift = AEVK_RSHIFT,
    LeftControl = AEVK_LCTRL,
    RightControl = AEVK_RCTRL,
    LeftAlt = AEVK_LALT,
    RightAlt = AEVK_RALT,
    
    // Navigation
    Insert = AEVK_INSERT,
    Delete = AEVK_DELETE,
    Home = AEVK_HOME,
    End = AEVK_END,
    PageUp = AEVK_PAGEUP,
    PageDown = AEVK_PAGEDOWN,
    
    // Numbers (top row)
    Alpha0 = AEVK_0,
    Alpha1 = AEVK_1,
    Alpha2 = AEVK_2,
    Alpha3 = AEVK_3,
    Alpha4 = AEVK_4,
    Alpha5 = AEVK_5,
    Alpha6 = AEVK_6,
    Alpha7 = AEVK_7,
    Alpha8 = AEVK_8,
    Alpha9 = AEVK_9,
    
    // Letters
    A = AEVK_A,
    B = AEVK_B,
    C = AEVK_C,
    D = AEVK_D,
    E = AEVK_E,
    F = AEVK_F,
    G = AEVK_G,
    H = AEVK_H,
    I = AEVK_I,
    J = AEVK_J,
    K = AEVK_K,
    L = AEVK_L,
    M = AEVK_M,
    N = AEVK_N,
    O = AEVK_O,
    P = AEVK_P,
    Q = AEVK_Q,
    R = AEVK_R,
    S = AEVK_S,
    T = AEVK_T,
    U = AEVK_U,
    V = AEVK_V,
    W = AEVK_W,
    X = AEVK_X,
    Y = AEVK_Y,
    Z = AEVK_Z,
    
    // Numpad
    Keypad0 = AEVK_NUMPAD0,
    Keypad1 = AEVK_NUMPAD1,
    Keypad2 = AEVK_NUMPAD2,
    Keypad3 = AEVK_NUMPAD3,
    Keypad4 = AEVK_NUMPAD4,
    Keypad5 = AEVK_NUMPAD5,
    Keypad6 = AEVK_NUMPAD6,
    Keypad7 = AEVK_NUMPAD7,
    Keypad8 = AEVK_NUMPAD8,
    Keypad9 = AEVK_NUMPAD9,
    KeypadMultiply = AEVK_NUM_MULTIPLY,
    KeypadPlus = AEVK_NUM_PLUS,
    KeypadMinus = AEVK_NUM_MINUS,
    KeypadPeriod = AEVK_NUM_PERIOD,
    KeypadDivide = AEVK_NUM_DIVIDE,
    
    // Function keys
    F1 = AEVK_F1,
    F2 = AEVK_F2,
    F3 = AEVK_F3,
    F4 = AEVK_F4,
    F5 = AEVK_F5,
    F6 = AEVK_F6,
    F7 = AEVK_F7,
    F8 = AEVK_F8,
    F9 = AEVK_F9,
    F10 = AEVK_F10,
    F11 = AEVK_F11,
    F12 = AEVK_F12,
    
    // Symbols
    Semicolon = AEVK_SEMICOLON,
    Slash = AEVK_SLASH,
    BackQuote = AEVK_BACKQUOTE,
    LeftBracket = AEVK_LBRACKET,
    Backslash = AEVK_BACKSLASH,
    RightBracket = AEVK_RBRACKET,
    Quote = AEVK_QUOTE,
    Equals = AEVK_EQUAL,
    Minus = AEVK_MINUS,
    Period = AEVK_PERIOD,
    Comma = AEVK_COMMA
};

} // namespace Engine
