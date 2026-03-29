/**
 * @file Application.cpp
 * @brief Implementation of Application class
 */
#include "Application.hpp"
#include "Time.hpp"
#include "ConfigManager.hpp"
#include <cstdio>

namespace Engine {

int Application::targetFrameRate = 60;
bool Application::isRunning = true;
std::unordered_set<std::string> Application::pauseRegistry;

void Application::Initialize(const std::string& configPath)
{
    ApplicationConfig config;
    if (!ConfigManager::ReadConfig(configPath, config))
    {
        std::printf("[Warning] Application: Failed to read config '%s'. Using built-in defaults.\n",
                    configPath.c_str());
    }
    targetFrameRate = config.targetFrameRate;
}

/*!
 * \brief [AUTO] Quit
 */
void Application::Quit() {
    isRunning = false;
}

/*!
 * \brief [AUTO] GetWindowWidth
 * \return 
 */
int Application::GetWindowWidth() {
    return AEGfxGetWindowWidth();
}

/*!
 * \brief [AUTO] GetWindowHeight
 * \return 
 */
int Application::GetWindowHeight() {
    return AEGfxGetWindowHeight();
}

/*!
 * \brief [AUTO] WindowExists
 * \return 
 */
bool Application::WindowExists() {
    return AESysDoesWindowExist() != 0;
}

/*!
 * \brief [AUTO] RegisterPause
 * \param key [in]
 */
void Application::RegisterPause(const std::string& key) {
    pauseRegistry.insert(key);
    Time::SetGameSpeed(0.0f);
}

/*!
 * \brief [AUTO] UnregisterPause
 * \param key [in]
 */
void Application::UnregisterPause(const std::string& key) {
    pauseRegistry.erase(key);
    if (pauseRegistry.empty()) {
        Time::SetGameSpeed(1.0f);
    }
}

/*!
 * \brief [AUTO] IsPaused
 * \return 
 */
bool Application::IsPaused() {
    return !pauseRegistry.empty();
}

/*!
 * \brief [AUTO] GetPauseCount
 * \return 
 */
size_t Application::GetPauseCount() {
    return pauseRegistry.size();
}

} // namespace Engine
