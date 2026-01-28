/**
 * @file Application.cpp
 * @brief Implementation of Application class
 */
#include "Application.hpp"
#include "Time.hpp"

namespace Engine {

int Application::targetFrameRate = 60;
bool Application::isRunning = true;
std::unordered_set<std::string> Application::pauseRegistry;

void Application::Quit() {
    isRunning = false;
}

int Application::GetWindowWidth() {
    return AEGfxGetWindowWidth();
}

int Application::GetWindowHeight() {
    return AEGfxGetWindowHeight();
}

bool Application::WindowExists() {
    return AESysDoesWindowExist() != 0;
}

void Application::RegisterPause(const std::string& key) {
    pauseRegistry.insert(key);
    Time::SetGameSpeed(0.0f);
}

void Application::UnregisterPause(const std::string& key) {
    pauseRegistry.erase(key);
    if (pauseRegistry.empty()) {
        Time::SetGameSpeed(1.0f);
    }
}

bool Application::IsPaused() {
    return !pauseRegistry.empty();
}

size_t Application::GetPauseCount() {
    return pauseRegistry.size();
}

} // namespace Engine
