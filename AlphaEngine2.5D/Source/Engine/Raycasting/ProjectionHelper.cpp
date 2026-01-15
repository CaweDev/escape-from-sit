/**
 * @file ProjectionHelper.cpp
 * @brief Implementation of ProjectionHelper class
 */
#include "ProjectionHelper.hpp"
#include <cmath>

namespace Engine {

ProjectionResult ProjectionHelper::ProjectToScreen(
    const Vector3& worldPos, 
    const Vector3& camPos, 
    f32 camRotationDeg, 
    f32 fovDeg,
    int screenWidth,
    int screenHeight) 
{
    ProjectionResult result;
    
    // 1. Calculate relative position
    f32 dx = worldPos.x - camPos.x;
    f32 dy = worldPos.y - camPos.y;
    result.distance = std::sqrt(dx * dx + dy * dy);
    
    if (result.distance < 0.1f) return result; // Too close

    // 2. Calculate angle relative to camera
    f32 angleToObj = std::atan2(dy, dx);
    f32 camAngleRad = camRotationDeg * 3.14159f / 180.0f;
    
    // Relative angle in radians
    f32 relAngle = angleToObj - camAngleRad;
    
    // Keep in [-PI, PI]
    while (relAngle < -3.14159f) relAngle += 6.28318f;
    while (relAngle > 3.14159f) relAngle -= 6.28318f;
    
    // 4. Fisheye correction (perpendicular distance)
    f32 perpDist = result.distance * std::cos(relAngle);
    result.perpDistance = perpDist;

    // 5. Calculate screen X and scale ALWAYS (even if off-screen/behind)
    f32 fovRad = fovDeg * 3.14159f / 180.0f;
    f32 cameraX = std::tan(relAngle) / std::tan(fovRad / 2.0f);
    result.screenX = (cameraX + 1.0f) * screenWidth / 2.0f;
    
    // Scale is based on perpDistance. Prevent division by zero.
    result.scale = static_cast<f32>(screenHeight) / (std::abs(perpDist) > 0.01f ? std::abs(perpDist) : 0.01f);

    // Screen Y is at center (standard raycaster - objects are at eye level)
    result.screenY = static_cast<f32>(screenHeight) / 2.0f;

    // 6. Check visibility for return status
    if (perpDist < 0.1f) return result; // Behind camera
    if (std::abs(relAngle) > (fovRad / 2.0f) * 1.5f) return result; 

    result.isVisible = true;
    return result;
}

ProjectionResult ProjectionHelper::ProjectToScreen(
    const Transform* target,
    const Transform* camera,
    f32 fovDeg,
    int screenWidth,
    int screenHeight)
{
    if (!target || !camera) return ProjectionResult();
    return ProjectToScreen(target->position, camera->position, camera->rotation, fovDeg, screenWidth, screenHeight);
}

} // namespace Engine
