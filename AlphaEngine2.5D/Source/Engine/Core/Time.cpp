/**
 * @file Time.cpp
 * @brief Implementation of Time class
 */
#include "Time.hpp"

namespace Engine {

f32 Time::deltaTime = 0;
f32 Time::fixedDeltaTime = 0.02f; // 50Hz physics
f32 Time::time = 0;
unsigned int Time::frameCount = 0;

void Time::Update() {
    deltaTime = static_cast<f32>(AEFrameRateControllerGetFrameTime());
    time += deltaTime;
    frameCount = AEFrameRateControllerGetFrameCount();
}

void Time::Reset() {
    deltaTime = 0;
    time = 0;
    frameCount = 0;
}

} // namespace Engine
