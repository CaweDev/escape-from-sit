/**
 * @file Rigidbody2D.cpp
 * @brief Implementation of Rigidbody2D physics body
 */
#include "Rigidbody2D.hpp"
#include "Collider2D.hpp"
#include "Physics2D.hpp"
#include "../Core/Transform.hpp"
#include "../Core/Time.hpp"
#include "../Core/GameObject.hpp"
#include "../Math/Mathf.hpp"

namespace Engine {

void Rigidbody2D::Awake() {
    collisionHeight = standingHeight;
    Physics2D::RegisterRigidbody(this);
}

void Rigidbody2D::OnDestroy() {
    Physics2D::UnregisterRigidbody(this);
}

void Rigidbody2D::FixedUpdate() {
    if (bodyType == RigidbodyType2D::Static) return;
    if (!transform) return;
    
    f32 dt = Time::fixedDeltaTime;
    if (dt <= 0) dt = 0.02f; // Default 50Hz physics
    
    // Apply gravity if not grounded
    if (!isGrounded && bodyType == RigidbodyType2D::Dynamic) {
        verticalVelocity -= Physics2D::gravity * gravityScale * dt;
    }
    
    // Apply accumulated forces (F = ma, so a = F/m)
    if (bodyType == RigidbodyType2D::Dynamic && mass > 0) {
        velocity += accumulatedForce * (dt / mass);
        accumulatedForce = Vector2::zero();
    }
    
    // Apply drag
    if (drag > 0) {
        f32 dragFactor = 1.0f - (drag * dt);
        if (dragFactor < 0) dragFactor = 0;
        velocity *= dragFactor;
    }
    
    // Update height
    if (!isGrounded) {
        currentHeight += verticalVelocity * dt;
        
        // Ground check
        if (currentHeight <= 0) {
            currentHeight = 0;
            verticalVelocity = 0;
            isGrounded = true;
        }
    }
    
    // Movement is handled by Physics2D::FixedUpdate() which processes collisions
}

void Rigidbody2D::AddForce(const Vector2& force) {
    if (bodyType != RigidbodyType2D::Dynamic) return;
    accumulatedForce += force;
}

void Rigidbody2D::AddImpulse(const Vector2& impulse) {
    if (bodyType != RigidbodyType2D::Dynamic) return;
    velocity += impulse;
}

void Rigidbody2D::SetVelocity(const Vector2& vel) {
    velocity = vel;
}

void Rigidbody2D::SetVelocity(f32 x, f32 y, f32 vertical) {
    velocity.x = x;
    velocity.y = y;
    verticalVelocity = vertical;
}

void Rigidbody2D::MovePosition(const Vector2& position) {
    if (!transform) return;
    transform->position.x = position.x;
    transform->position.y = position.y;
}

void Rigidbody2D::Jump(f32 jumpForce) {
    if (!isGrounded) return;
    
    verticalVelocity = jumpForce;
    isGrounded = false;
}

void Rigidbody2D::Crouch() {
    if (isCrouching) return;
    
    isCrouching = true;
    collisionHeight = crouchHeight;
    UpdateColliderHeight();
}

void Rigidbody2D::Stand() {
    if (!isCrouching) return;
    
    // TODO: Check if there's room to stand
    isCrouching = false;
    collisionHeight = standingHeight;
    UpdateColliderHeight();
}

void Rigidbody2D::ToggleCrouch() {
    if (isCrouching) Stand();
    else Crouch();
}

f32 Rigidbody2D::GetSpeed() const {
    return velocity.Magnitude();
}

Collider2D* Rigidbody2D::GetCollider() const {
    if (!gameObject) return nullptr;
    return gameObject->GetComponent<Collider2D>();
}

void Rigidbody2D::UpdateColliderHeight() {
    Collider2D* collider = GetCollider();
    if (collider) {
        collider->maxHeight = currentHeight + collisionHeight;
        collider->minHeight = currentHeight;
    }
}

} // namespace Engine
