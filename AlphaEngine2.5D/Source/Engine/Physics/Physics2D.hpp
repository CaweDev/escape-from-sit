/**
 * @file Physics2D.hpp
 * @brief Static physics manager for 2.5D collision detection and resolution
 */
#pragma once

#include "../Math/Vector2.hpp"
#include "Collision2D.hpp"
#include "AEEngine.h"
#include <vector>
#include <functional>

namespace Engine {

// Forward declarations
class Collider2D;
class Rigidbody2D;
class TileMap;

/**
 * @struct RaycastHit2D
 * @brief Information about a raycast hit
 */
struct RaycastHit2D {
    /** The collider that was hit */
    Collider2D* collider = nullptr;
    
    /** Point where the ray hit */
    Vector2 point;
    
    /** Normal of the surface at hit point */
    Vector2 normal;
    
    /** Distance from ray origin to hit point */
    f32 distance = 0.0f;
    
    /** Whether the raycast hit anything */
    operator bool() const { return collider != nullptr; }
};

/**
 * @class Physics2D
 * @brief Static class managing 2.5D physics simulation
 */
class Physics2D {
public:
    /** Global gravity (units per second squared) */
    static f32 gravity;
    
    /** Global flag for debug collider rendering (used by Minimap/Renderers) */
    static bool debugDrawEnabled;
    
    /** Reference to the active tilemap for wall collision */
    static TileMap* tileMap;

    // Simulation
    
    /** Run physics simulation step (called by Scene) */
    static void FixedUpdate();
    
    /** Register a collider for physics */
    static void RegisterCollider(Collider2D* collider);
    
    /** Unregister a collider */
    static void UnregisterCollider(Collider2D* collider);
    
    /** Register a rigidbody for physics */
    static void RegisterRigidbody(Rigidbody2D* rb);
    
    /** Unregister a rigidbody */
    static void UnregisterRigidbody(Rigidbody2D* rb);

    // Queries
    
    /** Cast a ray and get first hit */
    static RaycastHit2D Raycast(const Vector2& origin, const Vector2& direction, f32 maxDistance = 1000.0f);
    
    /** Cast a ray and get all hits */
    static std::vector<RaycastHit2D> RaycastAll(const Vector2& origin, const Vector2& direction, f32 maxDistance = 1000.0f);
    
    /** Check if a circle overlaps any collider */
    static Collider2D* OverlapCircle(const Vector2& center, f32 radius, f32 height = 1.0f, f32 heightPos = 0.0f);
    
    /** Get all colliders overlapping a circle */
    static std::vector<Collider2D*> OverlapCircleAll(const Vector2& center, f32 radius, f32 height = 1.0f, f32 heightPos = 0.0f);
    
    /** Check if a box overlaps any collider */
    static Collider2D* OverlapBox(const Vector2& center, const Vector2& size, f32 height = 1.0f, f32 heightPos = 0.0f);
    
    /** Get all colliders overlapping a box */
    static std::vector<Collider2D*> OverlapBoxAll(const Vector2& center, const Vector2& size, f32 height = 1.0f, f32 heightPos = 0.0f);

    // Collision resolution
    
    /** Check if a position is walkable on the tilemap */
    static bool IsWalkable(f32 x, f32 y);
    
    /** Try to move a rigidbody with collision */
    static Vector2 MoveWithCollision(Rigidbody2D* rb, const Vector2& movement);

    /** Get the list of all registered colliders (useful for debug drawing) */
    static const std::vector<Collider2D*>& GetColliders() { return colliders; }

    /**
     * @brief Renders 3D debug collider outlines on top of the current frame.
     * Uses Camera::main for projection. Call after all other renderers have drawn.
     */
    static void DrawDebugView();

private:
    Physics2D() = delete; // Static class
    
    static std::vector<Collider2D*> colliders;
    static std::vector<Rigidbody2D*> rigidbodies;
    
    /** Resolve collision between two colliders */
    static void ResolveCollision(Rigidbody2D* rb, Collider2D* other);
    
    /** Check tilemap collision and slide */
    static Vector2 TileMapCollision(const Vector2& position, const Vector2& movement, f32 radius);
};

} // namespace Engine
