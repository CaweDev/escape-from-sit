/**
 * @file FPSController.cpp
 * @brief Implementation of FPSController using Rigidbody2D
 */
#include "FPSController.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Math/Mathf.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Input/XboxController.hpp"
#include <iostream>
#include "Game/Player/Player.hpp"
#include "AEInput.h"

namespace Game {

void FPSController::Initialize() {
    // Get rigidbody reference
    rigidbody = gameObject->GetComponent<Rigidbody2D>();
    
    // Initialize view height
    currentViewHeight = eyeHeight;
    targetViewHeight = eyeHeight;
}

void FPSController::Update() {
    if (!transform || !rigidbody) return;

    f32 dt = Time::deltaTime;
    if (dt <= 0) dt = 0.016f;
    
    jumpCooldown -= dt;
    if (jumpCooldown < 0.0f) {
        jumpCooldown = 0.0f;
    }

    if (Input::IsCursorLocked()) { // if cursor is locked, allow mouse look
        // Get mouse delta from Input system (includes controller right stick)
        Vector2 mouseDelta = Input::GetMouseDelta();

        // Apply mouse/controller rotation
        transform->rotation -= mouseDelta.x * mouseSensitivity;

        // Apply vertical look (pitch)
        if (Camera::main) {
            Camera::main->pitch -= mouseDelta.y * mouseSensitivity * 5.f;
            // Clamp pitch to avoid extreme distortion (equivalent to screen height offset)
            // Adjusting horizon by +/- 0.8 * screenHeight is usually a safe limit
            Camera::main->pitch = Mathf::Clamp(Camera::main->pitch, -400.f, 400.f);
        }
    }
    // Keep angle in 0-360 range
    while (transform->rotation < 0) transform->rotation += 360.0f;
    while (transform->rotation >= 360) transform->rotation -= 360.0f;

    Vector2 worldVelocity = Vector2::zero();

    if (Input::IsCursorLocked()) {
        // Get local movement input (keyboard + controller left stick via GetAxis)
        Vector2 inputMovement = Vector2::zero();
        inputMovement.x = Input::GetAxis("Horizontal");
        inputMovement.y = Input::GetAxis("Vertical");

        Player* playerComp = gameObject->GetComponent<Player>();

        bool isMoving = (inputMovement.x != 0.0f || inputMovement.y != 0.0f);
        bool wantsSprint = Input::GetKey(KeyCode::LeftShift) || XboxController::GetButton(XboxButton::A, 0);
        bool canSprint = playerComp && playerComp->CanSprint() && !rigidbody->isCrouching;

        f32 speed = moveSpeed;

        if (wantsSprint && isMoving && playerComp && !rigidbody->isCrouching) {
            if (playerComp->CanSprint()) {
                // Can sprint - use sprint speed and drain stamina
                speed = sprintSpeed;
                playerComp->isSprinting = true;
                playerComp->DrainSprint(dt);
                Engine::SoundManager::PlaySFX("player_run", 0.7f);
            }
            else if (playerComp->GetStamina() > 0.0f) {
                // Stamina below threshold but not empty - continue draining to 0
                speed = moveSpeed; // Use normal speed
                playerComp->isSprinting = false;
                playerComp->DrainSprint(dt);
            }
            else {
                // Stamina completely empty
                playerComp->isSprinting = false;
            }
        }
        else {
            if (playerComp) {
                playerComp->isSprinting = false;
                playerComp->Regen(dt);
            }
        }

        // Cheat: infinite speed overrides normal speed (no stamina drain)
        if (Player::cheatInfiniteSpeed) {
            speed = sprintSpeed * 3.0f;
            if (playerComp) {
                playerComp->isSprinting = false;
                playerComp->ResetStamina();
            }
        }

        // Calculate movement direction based on current angle
        f32 angleRad = transform->rotation * 3.14159f / 180.0f;
        f32 cosA = cosf(angleRad);
        f32 sinA = sinf(angleRad);

        // Forward/back movement in look direction

        worldVelocity.x = (cosA * inputMovement.y * speed) // forward 
            + (sinA * inputMovement.x * speed); // strafe
        worldVelocity.y = (sinA * inputMovement.y * speed) // forward
            + (-cosA * inputMovement.x * speed); // strafe

        // Jumping (only when locked) - Space or Xbox A button
        bool spaceNow = Input::GetKey(KeyCode::Space) || XboxController::GetButton(XboxButton::A, 0);
        bool spaceDown = spaceNow && !prevSpace;

        if (spaceDown && jumpCooldown <= 0.0f) {
            rigidbody->Jump(jumpForce);
            jumpCooldown = 0.25f;
            Engine::SoundManager::PlaySFX("jump");
        }
        prevSpace = spaceNow;

        // Crouching (C or Xbox B button) (only when locked)
        if (Input::GetKeyDown(KeyCode::C) || XboxController::GetButtonDown(XboxButton::B, 0)) {
            rigidbody->ToggleCrouch();
        }

        // Set velocity on rigidbody (physics will handle collision)
        rigidbody->velocity = worldVelocity;
        // Trigger walking SFX occasionally when moving
        if (isMoving) {
            Engine::SoundManager::PlaySFX("player_walking", 0.4f);
        }
    }
    
	//std::cout << transform->position.x << ", " << transform->position.y << ", " << transform->position.z << std::endl;
    
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
    
}

} // namespace Game
