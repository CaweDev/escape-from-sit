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
#include "Engine/Core/ConfigManager.hpp"

namespace
{
    struct FPSControllerConfig : public Engine::IConfig
    {
        float fixedDtFallback = 0.016f;
        float pitchMultiplier = 5.0f;
        float pitchClampMin = -400.0f;
        float pitchClampMax = 400.0f;

        float speedBoostWalkBonus = 2.0f;
        float speedBoostSprintBonus = 3.0f;
        float cheatInfiniteSpeedMultiplier = 3.0f;

        float jumpCooldown = 0.25f;
        float jumpSfxIdleVolume = 1.0f;
        float jumpSfxWalkVolume = 0.65f;
        float jumpSfxRunVolume = 0.45f;

        float runLoopBaseVolume = 0.7f;
        float walkLoopBaseVolume = 0.4f;
        float movementLoopVolumeMultiplier = 2.05f;

        void FromJson(const Json::Value& j) override
        {
            fixedDtFallback = j.get("fixedDtFallback", fixedDtFallback).asFloat();
            pitchMultiplier = j.get("pitchMultiplier", pitchMultiplier).asFloat();
            pitchClampMin = j.get("pitchClampMin", pitchClampMin).asFloat();
            pitchClampMax = j.get("pitchClampMax", pitchClampMax).asFloat();

            speedBoostWalkBonus = j.get("speedBoostWalkBonus", speedBoostWalkBonus).asFloat();
            speedBoostSprintBonus = j.get("speedBoostSprintBonus", speedBoostSprintBonus).asFloat();
            cheatInfiniteSpeedMultiplier = j.get("cheatInfiniteSpeedMultiplier", cheatInfiniteSpeedMultiplier).asFloat();

            jumpCooldown = j.get("jumpCooldown", jumpCooldown).asFloat();
            jumpSfxIdleVolume = j.get("jumpSfxIdleVolume", jumpSfxIdleVolume).asFloat();
            jumpSfxWalkVolume = j.get("jumpSfxWalkVolume", jumpSfxWalkVolume).asFloat();
            jumpSfxRunVolume = j.get("jumpSfxRunVolume", jumpSfxRunVolume).asFloat();

            runLoopBaseVolume = j.get("runLoopBaseVolume", runLoopBaseVolume).asFloat();
            walkLoopBaseVolume = j.get("walkLoopBaseVolume", walkLoopBaseVolume).asFloat();
            movementLoopVolumeMultiplier = j.get("movementLoopVolumeMultiplier", movementLoopVolumeMultiplier).asFloat();
        }
    };

    FPSControllerConfig g_fpsControllerConfig;
    bool g_fpsControllerConfigLoaded = false;

    const FPSControllerConfig& GetFPSControllerConfig()
    {
        if (!g_fpsControllerConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/fps_controller.json", g_fpsControllerConfig);
            if (g_fpsControllerConfig.fixedDtFallback <= 0.0f) g_fpsControllerConfig.fixedDtFallback = 0.016f;
            if (g_fpsControllerConfig.pitchClampMin > g_fpsControllerConfig.pitchClampMax)
            {
                float t = g_fpsControllerConfig.pitchClampMin;
                g_fpsControllerConfig.pitchClampMin = g_fpsControllerConfig.pitchClampMax;
                g_fpsControllerConfig.pitchClampMax = t;
            }
            if (g_fpsControllerConfig.jumpCooldown < 0.0f) g_fpsControllerConfig.jumpCooldown = 0.0f;
            g_fpsControllerConfigLoaded = true;
        }
        return g_fpsControllerConfig;
    }
}

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

    const auto& cfg = GetFPSControllerConfig();
    f32 dt = Time::deltaTime;
    if (dt <= 0) dt = cfg.fixedDtFallback;
    
    jumpCooldown -= dt;
    if (jumpCooldown < 0.0f) {
        jumpCooldown = 0.0f;
    }

    // If cursor unlocked (e.g., menu open), ensure movement SFX loops are stopped and skip input
    if (!Input::IsCursorLocked()) {
        if (walkingLoopActive) {
            Engine::SoundManager::StopLoop("player_walking");
            walkingLoopActive = false;
        }
        if (runningLoopActive) {
            Engine::SoundManager::StopLoop("player_run");
            runningLoopActive = false;
        }
        return;
    }

    if (Input::IsCursorLocked()) { // if cursor is locked, allow mouse look
        // Get mouse delta from Input system (includes controller right stick)
        Vector2 mouseDelta = Input::GetMouseDelta();

        // Apply mouse/controller rotation
        transform->rotation -= mouseDelta.x * mouseSensitivity;

        // Apply vertical look (pitch)
        if (Camera::main) {
            Camera::main->pitch -= mouseDelta.y * mouseSensitivity * cfg.pitchMultiplier;
            Camera::main->pitch = Mathf::Clamp(Camera::main->pitch, cfg.pitchClampMin, cfg.pitchClampMax);
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

        f32 speed = moveSpeed;

        if (Player::hasSpeedBoost) {
            speed += cfg.speedBoostWalkBonus;
        }

        if (wantsSprint && isMoving && playerComp && !rigidbody->isCrouching) {
            if (playerComp->hasInfiniteStamina || playerComp->CanSprint()) {
                // Can sprint - use sprint speed and drain stamina
                speed = sprintSpeed;

                if (playerComp->hasSpeedBoost) {
                    speed += cfg.speedBoostSprintBonus;
                }

                playerComp->isSprinting = true;

                // Only drain stamina if no infinite powerup
                if (!playerComp->hasInfiniteStamina) {
                    playerComp->DrainSprint(dt);
                }
                if (playerComp->hasInfiniteStamina) {
                    playerComp->Regen(dt);
                }
                // Play running SFX once per start, avoid per-frame retrigger
                if (!runningLoopActive) {
                    Engine::SoundManager::PlayLoop("player_run", cfg.runLoopBaseVolume * cfg.movementLoopVolumeMultiplier);
                    runningLoopActive = true;
                }
            }
            else if (playerComp->GetStamina() > 0.0f) {
                // Stamina below threshold but not empty - continue draining to 0
                speed = moveSpeed; // Use normal speed
                if (playerComp->hasSpeedBoost) speed += cfg.speedBoostWalkBonus; // keep base speed boost
                playerComp->isSprinting = false;
                playerComp->DrainSprint(dt);
            }
            else {
                // Stamina completely empty
                if (playerComp->hasSpeedBoost) speed += cfg.speedBoostWalkBonus;
                playerComp->isSprinting = false;
            }
        }
        else {
            if (playerComp) {
                playerComp->isSprinting = false;
                playerComp->Regen(dt);
            }
            // stop running loop if it was playing
            if (runningLoopActive) {
                Engine::SoundManager::StopLoop("player_run");
                runningLoopActive = false;
            }
        }

        // Cheat: infinite speed overrides normal speed (no stamina drain)
        if (Player::cheatInfiniteSpeed) {
            speed = sprintSpeed * cfg.cheatInfiniteSpeedMultiplier;
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
            jumpCooldown = cfg.jumpCooldown;
            float jumpVol = cfg.jumpSfxIdleVolume;
            if (playerComp) {
                if (playerComp->isSprinting) jumpVol = cfg.jumpSfxRunVolume;
                else if (isMoving) jumpVol = cfg.jumpSfxWalkVolume;
            } else if (isMoving) {
                jumpVol = cfg.jumpSfxWalkVolume;
            }
            Engine::SoundManager::PlaySFX("jump", jumpVol);
        }
        prevSpace = spaceNow;

        // Crouching (C or Xbox B button) (only when locked)
        if (Input::GetKeyDown(KeyCode::C) || XboxController::GetButtonDown(XboxButton::B, 0)) {
            rigidbody->ToggleCrouch();
        }

        // Set velocity on rigidbody (physics will handle collision)
        rigidbody->velocity = worldVelocity;
        // Trigger walking SFX occasionally when moving
        if (isMoving && !playerComp->isSprinting) {
            // start walking loop if not already
            if (!walkingLoopActive) {
                Engine::SoundManager::PlayLoop("player_walking", cfg.walkLoopBaseVolume * cfg.movementLoopVolumeMultiplier);
                walkingLoopActive = true;
            }
        } else {
            if (walkingLoopActive) {
                Engine::SoundManager::StopLoop("player_walking");
                walkingLoopActive = false;
            }
        }
    }
    
	//std::cout << transform->position.x << ", " << transform->position.y << ", " << transform->position.z << std::endl;
    
    // Calculate target view height based on crouch state and jump
    // Z=0 is ground, Z=0.5 is eye level (standing), jumping adds to this
    f32 crouchOffset = rigidbody->isCrouching ? -0.25f : 0.0f; // Crouch lowers view
    targetViewHeight = eyeHeight + rigidbody->currentHeight + crouchOffset;
}

void FPSController::LateUpdate() {
    if (!transform || !rigidbody) return;
    
    const auto& cfg = GetFPSControllerConfig();
    f32 dt = Time::deltaTime;
    if (dt <= 0) dt = cfg.fixedDtFallback;
    
    // Smoothly interpolate view height for nice camera motion
    currentViewHeight = Mathf::Lerp(currentViewHeight, targetViewHeight, viewHeightSmoothSpeed * dt);
    
    // Apply view height to transform z (used for rendering perspective)
    transform->position.z = currentViewHeight;
}

void FPSController::OnDestroy() {
    
}

} // namespace Game
