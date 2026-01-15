/**
 * @file Collision2D.hpp
 * @brief Collision information struct for physics callbacks
 */
#pragma once

#include "../Math/Vector2.hpp"

namespace Engine {

// Forward declarations
class Collider2D;
class GameObject;

/**
 * @struct Collision2D
 * @brief Contains information about a collision between two colliders
 */
struct Collision2D {
    /** The collider that was hit */
    Collider2D* collider = nullptr;
    
    /** The GameObject that was hit */
    GameObject* gameObject = nullptr;
    
    /** The point of contact in world space */
    Vector2 contactPoint;
    
    /** The collision normal (pointing away from the other collider) */
    Vector2 normal;
    
    /** Penetration depth */
    f32 penetration = 0.0f;
};

} // namespace Engine
