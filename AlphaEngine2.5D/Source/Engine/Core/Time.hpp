/**
 * @file Time.hpp
 * @brief Time management with Unity-like API
 */
#pragma once

#include "AEEngine.h"

namespace Engine {

/**
 * @class Time
 * @brief Static class for time-related values
 */
class Time {
public:
    /* Gets Game Speed */
    static f32 GetGameSpeed();
    /* Sets Game Speed */
    static void SetGameSpeed(f32 newGameSpeed);

    /** Time in seconds since last frame, dependent on game speed */
    static f32 deltaTime;
    
    /* Time in seconds since last frame, independent from game speed */
    static f32 unscaledDeltaTime;

    /** Fixed timestep for physics (default 0.02 = 50Hz) */
    static f32 fixedDeltaTime;
    
    /** Total time elapsed since game start */
    static f32 time;
    
    /** Total number of frames rendered */
    static unsigned int frameCount;
    
    /** Update time values (called by engine each frame) */
    static void Update();
    
    /** Reset time values */
    static void Reset();

private:
    static f32 gameSpeed;
    Time() = delete; // Static class
};

} // namespace Engine
