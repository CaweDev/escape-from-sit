/**
 * @file FPSController.cpp
 * @brief Implementation of FPSController using Rigidbody2D
 */
#include "FPSController.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Math/Mathf.hpp"
#include "Engine/Graphics/Camera.hpp"
#include <iostream>
#include "Game/Player.hpp"
#include "AEInput.h"

namespace Game {

void FPSController::Initialize() {
    // Get rigidbody reference
    rigidbody = gameObject->GetComponent<Rigidbody2D>();
    
    // Initialize view height
    currentViewHeight = eyeHeight;
    targetViewHeight = eyeHeight;
    
    // Register for cursor lock
    Input::RegisterCursorLock("FPSController");
}

void FPSController::Update() {
    if (!transform || !rigidbody) return;

    f32 dt = Time::deltaTime;
    if (dt <= 0) dt = 0.016f;
    
    jumpCooldown -= dt;
    if (jumpCooldown < 0.0f) {
        jumpCooldown = 0.0f;
    }

    // Get mouse delta from Input system
    Vector2 mouseDelta = Input::GetMouseDelta();
    
    // Apply mouse rotation
    transform->rotation -= mouseDelta.x * mouseSensitivity;

    // Apply vertical look (pitch)
    if (Camera::main) {
        Camera::main->pitch -= mouseDelta.y * mouseSensitivity * 5.f;
        // Clamp pitch to avoid extreme distortion (equivalent to screen height offset)
        // Adjusting horizon by +/- 0.8 * screenHeight is usually a safe limit
        Camera::main->pitch = Mathf::Clamp(Camera::main->pitch, -400.f, 400.f);
    }
    
    // Keyboard rotation
    if (Input::GetKey(KeyCode::LeftArrow)) transform->rotation -= rotateSpeed * dt;
    if (Input::GetKey(KeyCode::RightArrow)) transform->rotation += rotateSpeed * dt;
    
    // Keep angle in 0-360 range
    while (transform->rotation < 0) transform->rotation += 360.0f;
    while (transform->rotation >= 360) transform->rotation -= 360.0f;
    
    // Get local movement input
    Vector2 inputMovement = Vector2::zero();
    
    if (Input::GetKey(KeyCode::W) || Input::GetKey(KeyCode::UpArrow)) inputMovement.y += 1.0f;
    if (Input::GetKey(KeyCode::S) || Input::GetKey(KeyCode::DownArrow)) inputMovement.y -= 1.0f;
    if (Input::GetKey(KeyCode::A)) inputMovement.x -= 1.0f;
    if (Input::GetKey(KeyCode::D)) inputMovement.x += 1.0f;
    
    Player* playerComp = gameObject->GetComponent<Player>();
    
    bool isMoving = (inputMovement.x != 0.0f || inputMovement.y != 0.0f);
    bool wantsSprint = Input::GetKey(KeyCode::LeftShift);
    bool canSprint = playerComp && playerComp->CanSprint() && !rigidbody->isCrouching;

    f32 speed = moveSpeed;

    if (wantsSprint && isMoving && playerComp && !rigidbody->isCrouching) {
        if (playerComp->CanSprint()) {
            speed = sprintSpeed;
            playerComp->isSprinting = true;
            playerComp->DrainSprint(dt);
        }
        else {
            playerComp->isSprinting = false;
        }
    }
    else {
        if (playerComp) {
            playerComp->isSprinting = false;
            playerComp->Regen(dt);
        }
    }



    // Calculate movement direction based on current angle
    f32 angleRad = transform->rotation * 3.14159f / 180.0f;
    f32 cosA = cosf(angleRad);
    f32 sinA = sinf(angleRad);
    
    // Forward/back movement in look direction
    Vector2 worldVelocity = Vector2::zero();
    worldVelocity.x = (cosA * inputMovement.y * speed) // forward 
                    + (sinA * inputMovement.x * speed); // strafe
    worldVelocity.y = (sinA * inputMovement.y * speed) // forward
                    + (-cosA * inputMovement.x * speed); // strafe
   
    // Set velocity on rigidbody (physics will handle collision)
    rigidbody->velocity = worldVelocity;
    
    // Jumping
    bool spaceNow = Input::GetKey(KeyCode::Space);
    bool spaceDown = spaceNow && !prevSpace;

    if (spaceDown && jumpCooldown <= 0.0f ) {
        rigidbody->Jump(jumpForce);
        jumpCooldown = 0.25f;
    }
    prevSpace = spaceNow;
	//std::cout << transform->position.x << ", " << transform->position.y << ", " << transform->position.z << std::endl;
    
    // Crouching (Ctrl or C)
    if (Input::GetKeyDown(KeyCode::LeftControl) || Input::GetKeyDown(KeyCode::C)) {
        rigidbody->ToggleCrouch();
    }
    
    // Calculate target view height based on crouch state and jump
    // Z=0 is ground, Z=0.5 is eye level (standing), jumping adds to this
    f32 crouchOffset = rigidbody->isCrouching ? -0.25f : 0.0f; // Crouch lowers view
    targetViewHeight = eyeHeight + rigidbody->currentHeight + crouchOffset;
    f32 actualSpeed = sqrtf(rigidbody->velocity.x * rigidbody->velocity.x +
        rigidbody->velocity.y * rigidbody->velocity.y);
    
   
}

void FPSController::LateUpdate() {
    if (!transform || !rigidbody) return;
    
    f32 dt = Time::deltaTime;
    if (dt <= 0) dt = 0.016f;
    
    // Smoothly interpolate view height for nice camera motion
    currentViewHeight = Mathf::Lerp(currentViewHeight, targetViewHeight, viewHeightSmoothSpeed * dt);
    
    // Apply view height to transform z (used for rendering perspective)
    transform->position.z = currentViewHeight;
}

void FPSController::OnDestroy() {
    Input::UnregisterCursorLock("FPSController");
}

} // namespace Game
