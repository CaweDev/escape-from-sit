/**
 * @file Camera.hpp
 * @brief Main camera for 2D rendering
 */
#pragma once

#include "Engine/Core/Component.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Color.hpp"
#include "AEEngine.h"

namespace Engine {

/**
 * @class Camera
 * @brief Component for camera control and viewport management
 */
class Camera : public Component {
public:
    /** Background clear color */
    Color backgroundColor = Color(0.2f, 0.2f, 0.2f, 1.0f);
    
    /** Orthographic size (half-height in world units) */
    f32 orthographicSize = 5.0f;
    
    f32 fov = 75.0f;

    /** Vertical rotation (pitch) for 2.5D dynamic horizon */
    f32 pitch = 0.0f;

    /** Static reference to main camera */
    static Camera* main;

    void Awake() override;
    
    void OnDestroy() override;

    /** Apply camera settings (called before rendering) */
    void Apply();
    
    /** Convert screen position to world position */
    Vector2 ScreenToWorldPoint(const Vector2& screenPos);
    
    /** Convert world position to screen position */
    Vector2 WorldToScreenPoint(const Vector2& worldPos);
    
    /** Get viewport width in world units */
    f32 GetViewportWidth() const;
    
    /** Get viewport height in world units */
    f32 GetViewportHeight() const;
};

} // namespace Engine
