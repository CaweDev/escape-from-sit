/**
 * @file Application.hpp
 * @brief Application management
 */
#pragma once

#include "AEEngine.h"

namespace Engine {

/**
 * @class Application
 * @brief Static class for application-level operations
 */
class Application {
public:
    /** Target frame rate (0 = unlimited) */
    static int targetFrameRate;
    
    /** Whether the application is running */
    static bool isRunning;
    
    /** Request application quit */
    static void Quit();
    
    /** Get window width */
    static int GetWindowWidth();
    
    /** Get window height */
    static int GetWindowHeight();
    
    /** Check if window exists */
    static bool WindowExists();

private:
    Application() = delete; // Static class
};

} // namespace Engine
