/**
 * @file Camera.cpp
 * @brief Implementation of Camera class
 */
#include "Camera.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <cstdio>

namespace Engine {

Camera* Camera::main = nullptr;

// Cached config applied to every camera that becomes main
static CameraConfig s_cameraConfig;

void Camera::Initialize(const std::string& configPath)
{
    if (!ConfigManager::ReadConfig(configPath, s_cameraConfig))
    {
        std::printf("[Warning] Camera: Failed to read config '%s'. Using built-in defaults.\n",
                    configPath.c_str());
    }
}

/*!
 * \brief [AUTO] Awake
 */
void Camera::Awake() {
    // Set as main camera if none exists
    if (!main) {
        main = this;
        // Apply config defaults to this camera instance
        backgroundColor  = s_cameraConfig.backgroundColor;
        orthographicSize = s_cameraConfig.orthographicSize;
        fov              = s_cameraConfig.fov;
        pitch            = s_cameraConfig.pitch;
        fogColor         = s_cameraConfig.fogColor;
        fogDistance      = s_cameraConfig.fogDistance;
        fogMinVisibility = s_cameraConfig.fogMinVisibility;
        fogMaxVisibility = s_cameraConfig.fogMaxVisibility;
        fogPrecision     = s_cameraConfig.fogPrecision;
        fogEnabled       = s_cameraConfig.fogEnabled;
    }
}

/*!
 * \brief [AUTO] OnDestroy
 */
void Camera::OnDestroy() {
    if (main == this) {
        main = nullptr;
    }
}

/*!
 * \brief [AUTO] Apply
 */
void Camera::Apply() {
    if (!transform) return;
    
    // Set background color
    AEGfxSetBackgroundColor(backgroundColor.r, backgroundColor.g, backgroundColor.b);
    
    // Set camera position
    AEGfxSetCamPosition(transform->position.x, transform->position.y);
}

/*!
 * \brief [AUTO] ScreenToWorldPoint
 * \param screenPos [in]
 * \return 
 */
Vector2 Camera::ScreenToWorldPoint(const Vector2& screenPos) {
    f32 winWidth = static_cast<f32>(AEGfxGetWindowWidth());
    f32 winHeight = static_cast<f32>(AEGfxGetWindowHeight());
    
    // Convert from screen coords (0,0 top-left) to world coords
    f32 worldX = (screenPos.x - winWidth / 2.0f);
    f32 worldY = (winHeight / 2.0f - screenPos.y);
    
    // Apply camera offset
    if (transform) {
        worldX += transform->position.x;
        worldY += transform->position.y;
    }
    
    return Vector2(worldX, worldY);
}

/*!
 * \brief [AUTO] WorldToScreenPoint
 * \param worldPos [in]
 * \return 
 */
Vector2 Camera::WorldToScreenPoint(const Vector2& worldPos) {
    f32 winWidth = static_cast<f32>(AEGfxGetWindowWidth());
    f32 winHeight = static_cast<f32>(AEGfxGetWindowHeight());
    
    f32 offsetX = transform ? transform->position.x : 0;
    f32 offsetY = transform ? transform->position.y : 0;
    
    f32 screenX = (worldPos.x - offsetX) + winWidth / 2.0f;
    f32 screenY = winHeight / 2.0f - (worldPos.y - offsetY);
    
    return Vector2(screenX, screenY);
}

/*!
 * \brief [AUTO] GetViewportWidth
 * \return 
 */
f32 Camera::GetViewportWidth() const {
    f32 aspect = static_cast<f32>(AEGfxGetWindowWidth()) / 
                   static_cast<f32>(AEGfxGetWindowHeight());
    return orthographicSize * 2.0f * aspect;
}

/*!
 * \brief [AUTO] GetViewportHeight
 * \return 
 */
f32 Camera::GetViewportHeight() const {
    return orthographicSize * 2.0f;
}

} // namespace Engine
