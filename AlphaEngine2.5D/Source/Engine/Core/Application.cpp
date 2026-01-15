/**
 * @file Application.cpp
 * @brief Implementation of Application class
 */
#include "Application.hpp"

namespace Engine {

int Application::targetFrameRate = 60;
bool Application::isRunning = true;

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

} // namespace Engine
