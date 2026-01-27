/**
 * @file ProjectionHelper.cpp
 * @brief Implementation of ProjectionHelper class
 */
#include "ProjectionHelper.hpp"
#include "Engine/Graphics/Camera.hpp"


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
    
    // 1. Calculate relative position (including height)
    // Z=0 is ground, Z=0.5 is eye level (wall center), Z=1.0 is ceiling
    f32 dx = worldPos.x - camPos.x;
    f32 dy = worldPos.y - camPos.y;
    f32 dz = worldPos.z - camPos.z;
    
    // 2D distance for angle calculations
    f32 dist2D = std::sqrt(dx * dx + dy * dy);
    
    // 3D distance for accurate scaling
    result.distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    
    if (dist2D < 0.1f) return result; // Too close

    // 2. Calculate angle relative to camera (horizontal plane)
    f32 angleToObj = std::atan2(dy, dx);
    f32 camAngleRad = camRotationDeg * 3.14159f / 180.0f;
    
    // Relative angle in radians
    f32 relAngle = angleToObj - camAngleRad;
    
    // Keep in [-PI, PI]
    while (relAngle < -3.14159f) relAngle += 6.28318f;
    while (relAngle > 3.14159f) relAngle -= 6.28318f;
    
    // 4. Fisheye correction (perpendicular distance on horizontal plane)
    f32 perpDist = dist2D * std::cos(relAngle);
    result.perpDistance = perpDist;

    // 5. Calculate screen X and scale ALWAYS (even if off-screen/behind)
    f32 fovRad = fovDeg * 3.14159f / 180.0f;
    f32 planeLength = std::tan(fovRad / 2.0f);
    f32 cameraX = std::tan(relAngle) / planeLength;
    result.screenX = (cameraX + 1.0f) * screenWidth / 2.0f;
    
    // Scale uses aspect-ratio-corrected formula matching TileMapRenderer
    // This ensures sprites scale consistently with walls at the same distance
    f32 safePerpDist = (std::abs(perpDist) > 0.01f) ? std::abs(perpDist) : 0.01f;
    result.scale = static_cast<f32>(screenWidth) / (safePerpDist * planeLength * 2.0f);

    // Screen Y accounts for height difference (objects above camera appear higher on screen)
    // Plus the dynamic horizon (pitch)
    // Use the same height offset formula as TileMapRenderer for consistency
    f32 projectedHeight = dz * result.scale;
    f32 pitchOffset = (Camera::main) ? Camera::main->pitch : 0.0f;
    result.screenY = (static_cast<f32>(screenHeight) / 2.0f) + pitchOffset - projectedHeight;

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
