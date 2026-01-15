/**
 * @file Rigidbody2D.hpp
 * @brief Physics body component with velocity, gravity, and height support
 */
#pragma once

#include "../Core/Component.hpp"
#include "../Math/Vector2.hpp"
#include "AEEngine.h"

namespace Engine {

// Forward declaration
class Collider2D;

/**
 * @enum RigidbodyType2D
 * @brief Types of rigidbody simulation
 */
enum class RigidbodyType2D {
    Dynamic,    // Fully simulated with forces and collisions
    Kinematic,  // Moved by code, affects others but not affected by forces
    Static      // Never moves (use for walls, platforms)
};

/**
 * @class Rigidbody2D
 * @brief Physics body component for 2.5D movement with height
 */
class Rigidbody2D : public Component {
public:
    // Physics properties
    
    /** Current velocity on the floor plane */
    Vector2 velocity;
    
    /** Current vertical velocity (for jumping) */
    f32 verticalVelocity = 0.0f;
    
    /** Mass of the rigidbody */
    f32 mass = 1.0f;
    
    /** Linear drag (velocity damping) */
    f32 drag = 0.0f;
    
    /** Gravity multiplier (1.0 = normal gravity) */
    f32 gravityScale = 1.0f;
    
    /** Type of rigidbody simulation */
    RigidbodyType2D bodyType = RigidbodyType2D::Dynamic;
    
    /** If true, rotation is not affected by physics */
    bool freezeRotation = true;

    // Height/crouch properties
    
    /** Current height above ground */
    f32 currentHeight = 0.0f;
    
    /** Height of player when standing */
    f32 standingHeight = 1.8f;
    
    /** Height of player when crouching */
    f32 crouchHeight = 0.9f;
    
    /** Current collision height (changes when crouching) */
    f32 collisionHeight = 1.8f;
    
    /** Whether currently crouching */
    bool isCrouching = false;
    
    /** Whether currently on the ground */
    bool isGrounded = true;

    Rigidbody2D() = default;

    void Awake() override;
    
    void OnDestroy() override;
    
    void FixedUpdate() override;

    // Movement methods
    
    /** Add a force to the rigidbody (affected by mass) */
    void AddForce(const Vector2& force);
    
    /** Add an instant velocity change (ignores mass) */
    void AddImpulse(const Vector2& impulse);
    
    /** Set velocity directly */
    void SetVelocity(const Vector2& vel);
    
    /** Set velocity directly (including vertical) */
    void SetVelocity(f32 x, f32 y, f32 vertical = 0.0f);
    
    /** Move to a position (kinematic movement) */
    void MovePosition(const Vector2& position);

    // Jump and crouch
    
    /** Jump with specified force (only works if grounded) */
    void Jump(f32 jumpForce);
    
    /** Start crouching (reduces collision height) */
    void Crouch();
    
    /** Stop crouching (restores collision height) */
    void Stand();
    
    /** Toggle crouch state */
    void ToggleCrouch();

    // Queries
    
    /** Get current speed (magnitude of velocity) */
    f32 GetSpeed() const;
    
    /** Get the attached collider (if any) */
    Collider2D* GetCollider() const;

private:
    /** Accumulated force for this physics step */
    Vector2 accumulatedForce;
    
    /** Update collider height based on crouch state */
    void UpdateColliderHeight();
};

} // namespace Engine
