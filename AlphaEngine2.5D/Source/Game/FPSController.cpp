/**
 * @file FPSController.cpp
 * @brief Implementation of FPSController using Rigidbody2D
 */
#include "FPSController.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Math/Mathf.hpp"
#include <iostream>

namespace Game {

void FPSController::Initialize() {
    // Get rigidbody reference
    rigidbody = gameObject->GetComponent<Rigidbody2D>();
    
    // Initialize view height
    if (rigidbody) {
        currentViewHeight = rigidbody->standingHeight * eyeHeightRatio;
        targetViewHeight = currentViewHeight;
    }
    
    // Hide cursor
    ShowCursor(FALSE);
    
    // Get window handle and center
    hwnd = GetActiveWindow();
    UpdateCenter();
    CenterCursor();
}

void FPSController::Update() {
    if (!transform || !rigidbody) return;

    f32 dt = Time::deltaTime;
    if (dt <= 0) dt = 0.016f;
    
    // Skip first frame to avoid spinning (cursor not centered yet)
    if (firstFrame) {
        firstFrame = false;
        CenterCursor();
        return;
    }
    
    // Get current cursor position
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    
    // Calculate delta from center
    f32 mouseDeltaX = static_cast<f32>(cursorPos.x - centerX);
    f32 mouseDeltaY = static_cast<f32>(cursorPos.y - centerY);
    
    // Re-center cursor immediately
    CenterCursor();
    
    // Apply mouse rotation
    transform->rotation += mouseDeltaX * mouseSensitivity;
    
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
    
    // Calculate movement direction based on current angle
    f32 angleRad = transform->rotation * 3.14159f / 180.0f;
    f32 cosA = cosf(angleRad);
    f32 sinA = sinf(angleRad);
    
    // Forward/back movement in look direction
    Vector2 worldVelocity = Vector2::zero();
    worldVelocity.x = (cosA * inputMovement.y * moveSpeed) // forward 
                    + (-sinA * inputMovement.x * moveSpeed); // strafe
    worldVelocity.y = (sinA * inputMovement.y * moveSpeed) // forward
                    + (cosA * inputMovement.x * moveSpeed); // strafe
   
    // Set velocity on rigidbody (physics will handle collision)
    rigidbody->velocity = worldVelocity;
    
    // Jumping
    if (Input::GetKeyDown(KeyCode::Space) && rigidbody->isGrounded) {
        rigidbody->Jump(jumpForce);
    }
    
    // Crouching (Ctrl or C)
    if (Input::GetKeyDown(KeyCode::LeftControl) || Input::GetKeyDown(KeyCode::C)) {
        rigidbody->ToggleCrouch();
    }
    
    // Calculate target view height based on crouch state and jump
    f32 bodyHeight = rigidbody->isCrouching ? rigidbody->crouchHeight : rigidbody->standingHeight;
    targetViewHeight = rigidbody->currentHeight + (bodyHeight * eyeHeightRatio);
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
    ShowCursor(TRUE);
}

void FPSController::UpdateCenter() {
    if (!hwnd) hwnd = GetActiveWindow();
    if (hwnd) {
        RECT rect;
        GetWindowRect(hwnd, &rect);
        centerX = (rect.left + rect.right) / 2;
        centerY = (rect.top + rect.bottom) / 2;
    }
}

void FPSController::CenterCursor() {
    SetCursorPos(centerX, centerY);
}

} // namespace Game
