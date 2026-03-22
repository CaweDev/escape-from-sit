/**
 * @file Application.hpp
 * @brief Application management
 */
#pragma once

#include "AEEngine.h"
#include "ConfigManager.hpp"
#include <string>
#include <unordered_set>

namespace Engine {

/**
 * @brief IConfig implementation that deserialises application.json.
 */
struct ApplicationConfig : public IConfig
{
    int targetFrameRate = 60; ///< Target frames per second (0 = unlimited).

    void FromJson(const Json::Value& j) override
    {
        targetFrameRate = j.get("targetFrameRate", targetFrameRate).asInt();
    }
};

/**
 * @class Application
 * @brief Static class for application-level operations
 */
class Application {
public:
    /** Target frame rate (0 = unlimited) */
    static int targetFrameRate;

    /**
     * @brief Load application configuration from a JSON file.
     * @param configPath Path to application.json. Defaults to the engine config location.
     */
    static void Initialize(const std::string& configPath = "Assets/Config/Engine/application.json");
    
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

    // Pause Registration System
    
    /** Register a pause request (game pauses while any request is active) */
    static void RegisterPause(const std::string& key);
    
    /** Unregister a pause request */
    static void UnregisterPause(const std::string& key);
    
    /** Check if game is currently paused */
    static bool IsPaused();
    
    /** Get count of active pause requests (for debugging) */
    static size_t GetPauseCount();

private:
    Application() = delete; // Static class
    
    /** Registry of active pause requests */
    static std::unordered_set<std::string> pauseRegistry;
};

} // namespace Engine
