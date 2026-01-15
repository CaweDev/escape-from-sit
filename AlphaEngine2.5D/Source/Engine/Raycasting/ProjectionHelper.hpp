/**
 * @file ProjectionHelper.hpp
 * @brief Static helper for 2.5D raycasting projection
 */
#pragma once

#include "../Math/Vector3.hpp"
#include "../Math/Mathf.hpp"
#include "../Core/Transform.hpp"
#include "AEEngine.h"
#include <cmath>

namespace Engine {

/**
 * @struct ProjectionResult
 * @brief Results of a 2.5D projection calculation
 */
struct ProjectionResult {
    bool isVisible = false;
    f32 screenX = 0.0f;
    f32 screenY = 0.0f;
    f32 scale = 1.0f;
    f32 distance = 0.0f;      // Euclidean distance
    f32 perpDistance = 0.0f;  // Fisheye-corrected distance
};

/**
 * @class ProjectionHelper
 * @brief Utility for projecting 3D positions into 2.5D screen space
 */
class ProjectionHelper {
public:
    static ProjectionResult ProjectToScreen(
        const Vector3& worldPos, 
        const Vector3& camPos, 
        f32 camRotationDeg, 
        f32 fovDeg,
        int screenWidth,
        int screenHeight);
    
    /** Overload for Transform objects */
    static ProjectionResult ProjectToScreen(
        const Transform* target,
        const Transform* camera,
        f32 fovDeg,
        int screenWidth,
        int screenHeight);
};

} // namespace Engine
