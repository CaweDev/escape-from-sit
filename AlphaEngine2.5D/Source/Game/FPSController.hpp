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
#include <Windows.h>
#include "Game/Player.hpp"
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
    f32 jumpForce = 5.0f;
    
    /** Eye height as ratio of body height (0.9 = eyes at 90% of body height) */
    f32 eyeHeightRatio = 0.9f;
    
    /** Smoothing speed for camera height changes */
    f32 viewHeightSmoothSpeed = 15.0f;
    
    /** Cached rigidbody reference */
    Rigidbody2D* rigidbody = nullptr;

    void Initialize() override;
    
    void Update() override;
    
    void LateUpdate() override;
    
    void OnDestroy() override;

private:
    HWND hwnd = nullptr;
    int centerX = 0;
    int centerY = 0;
    
    /** Skip first frame to avoid spinning on spawn */
    bool firstFrame = true;
    
    /** Current smoothed view height */
    f32 currentViewHeight = 1.6f;
    
    /** Target view height (based on crouch/jump state) */
    f32 targetViewHeight = 1.6f;
    
    void UpdateCenter();
    
    void CenterCursor();
};

} // namespace Game
