/**
 * @file FPSController.hpp
 * @brief First-person controller using Rigidbody2D physics
 */
#pragma once

#include "Engine/Core/Component.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include <cmath>
#include "Game/Player/Player.hpp"
namespace Game {

using namespace Engine;

/**
 * @class FPSController
 * @brief First-person movement with WASD, mouse look, jumping, and crouching
 */
class FPSController : public Component {
public:
    REQUIRE_COMPONENT(Rigidbody2D)
    

    /** Movement speed in units per second */
    f32 moveSpeed = 3.0f;
    
    f32 sprintSpeed = 6.0f;

    /** Rotation speed in degrees per second */
    f32 rotateSpeed = 60.0f;
    
    /** Mouse sensitivity */
    f32 mouseSensitivity = 0.15f;
    
    /** Jump force */
    f32 jumpForce = 3.0f;
    
    /** Eye height in world units (0.5 = center of wall, standard eye level) */
    f32 eyeHeight = 0.5f;
    
    /** Smoothing speed for camera height changes */
    f32 viewHeightSmoothSpeed = 15.0f;
    
    /** Cached rigidbody reference */
    Rigidbody2D* rigidbody = nullptr;

    // Audio loop state for movement SFX
    bool walkingLoopActive = false;
    bool runningLoopActive = false;

    void Initialize() override;
    
    void Update() override;
    
    void LateUpdate() override;
    
    void OnDestroy() override;
 
private:
    bool prevSpace = false;

    f32 jumpCooldown = 0.0f;
    
    /** Current smoothed view height */
    f32 currentViewHeight = 0.5f;
    
    /** Target view height (based on crouch/jump state) */
    f32 targetViewHeight = 0.5f;
};

} // namespace Game
