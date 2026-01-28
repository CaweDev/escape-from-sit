/**
 * @file Time.cpp
 * @brief Implementation of Time class
 */
#include "Time.hpp"

namespace Engine {

f32 Time::gameSpeed = 1.f;
f32 Time::deltaTime = 0;
f32 Time::unscaledDeltaTime = 0.02f;
f32 Time::fixedDeltaTime = 0.02f; // 50Hz physics
f32 Time::time = 0;
unsigned int Time::frameCount = 0;

f32 Time::GetGameSpeed()
{
    return gameSpeed;
}

void Time::SetGameSpeed(f32 newGameSpeed)
{
    gameSpeed = newGameSpeed;
}

void Time::Update() {
    unscaledDeltaTime = static_cast<f32>(AEFrameRateControllerGetFrameTime());
    time += unscaledDeltaTime;
    deltaTime = unscaledDeltaTime * gameSpeed;
    frameCount = AEFrameRateControllerGetFrameCount();
}

void Time::Reset() {
    deltaTime = 0;
    time = 0;
    frameCount = 0;
}

} // namespace Engine
