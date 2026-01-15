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
    /** Time in seconds since last frame */
    static f32 deltaTime;
    
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
    Time() = delete; // Static class
};

} // namespace Engine
