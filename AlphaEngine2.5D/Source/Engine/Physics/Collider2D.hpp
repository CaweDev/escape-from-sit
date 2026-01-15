/**
 * @file Collider2D.hpp
 * @brief Base class for all 2D colliders with height support
 */
#pragma once

#include "../Core/Component.hpp"
#include "../Math/Vector2.hpp"
#include "Collision2D.hpp"
#include "AEEngine.h"

namespace Engine {

/**
 * @enum CollisionLayer
 * @brief Layers for filtering collisions between colliders
 */
enum class CollisionLayer : u32 {
    Default = 0,      // Standard colliders (players, enemies, etc.)
    TileMap = 1,      // Tilemap collider (handled separately)
    Trigger = 2,      // Trigger volumes
    Projectile = 3,   // Bullets, missiles, etc.
    Player = 4,       // Player-specific layer
    Enemy = 5,        // Enemy-specific layer
    // Add more as needed (up to 31 for 32-bit mask)
};

/**
 * @brief Convert layer to bitmask value
 */
inline u32 LayerToMask(CollisionLayer layer) {
    return 1u << static_cast<u32>(layer);
}

/**
 * @struct Bounds2D
 * @brief Axis-aligned bounding box for collision detection
 */
struct Bounds2D {
    Vector2 min;
    Vector2 max;
    
    Vector2 Center() const { return (min + max) * 0.5f; }
    Vector2 Size() const { return max - min; }
    Vector2 Extents() const { return (max - min) * 0.5f; }
    
    bool Overlaps(const Bounds2D& other) const {
        return !(max.x < other.min.x || min.x > other.max.x ||
                 max.y < other.min.y || min.y > other.max.y);
    }
};

/**
 * @class Collider2D
 * @brief Base class for 2D colliders with height support for 2.5D games
 */
class Collider2D : public Component {
public:
    /** Offset from transform position */
    Vector2 offset;
    
    /** If true, this collider is a trigger (no physical collision response) */
    bool isTrigger = false;
    
    /** Which layer this collider belongs to */
    CollisionLayer layer = CollisionLayer::Default;
    
    /** Bitmask of layers this collider can collide with (default: all except TileMap) */
    u32 collisionMask = ~LayerToMask(CollisionLayer::TileMap);
    
    /** Bottom of the collider in world height units */
    f32 minHeight = 0.0f;
    
    /** Top of the collider in world height units */
    f32 maxHeight = 2.0f;
    
    /** Check if this collider should collide with another based on layers */
    bool CanCollideWith(const Collider2D* other) const {
        if (!other) return false;
        u32 otherMask = LayerToMask(other->layer);
        return (collisionMask & otherMask) != 0;
    }

    Collider2D() = default;
    virtual ~Collider2D() = default;

    /** Called when component is created - registers with Physics2D */
    void Awake() override;
    
    /** Called when component is destroyed - unregisters from Physics2D */
    void OnDestroy() override;

    /** Get the world-space center of the collider */
    virtual Vector2 GetCenter() const;
    
    /** Get the axis-aligned bounding box */
    virtual Bounds2D GetBounds() const = 0;
    
    /** Check if this collider overlaps with another */
    virtual bool Overlaps(const Collider2D* other) const;
    
    /** Check if height ranges overlap (for 2.5D collision) */
    bool HeightOverlaps(const Collider2D* other) const;
    
    /** Check if a point at given height overlaps this collider's height range */
    bool HeightContains(f32 height, f32 objectHeight) const;

    // Collision callbacks (override in derived components)
    
    /** Called when collision starts */
    virtual void OnCollisionEnter2D(const Collision2D& collision) {}
    
    /** Called every frame while colliding */
    virtual void OnCollisionStay2D(const Collision2D& collision) {}
    
    /** Called when collision ends */
    virtual void OnCollisionExit2D(const Collision2D& collision) {}
    
    /** Called when trigger overlap starts */
    virtual void OnTriggerEnter2D(Collider2D* other) {}
    
    /** Called every frame while overlapping trigger */
    virtual void OnTriggerStay2D(Collider2D* other) {}
    
    /** Called when trigger overlap ends */
    virtual void OnTriggerExit2D(Collider2D* other) {}
};

} // namespace Engine
