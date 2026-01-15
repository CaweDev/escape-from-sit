/**
 * @file TileMapCollider2D.hpp
 * @brief Collider component that generates collision from a TileMap
 */
#pragma once

#include "Collider2D.hpp"
#include "../TileMap/TileMap.hpp"

namespace Engine {

/**
 * @class TileMapCollider2D
 * @brief Generates collision surfaces from TileMap wall tiles
 * 
 * Attach this to the same GameObject as a TileMap component.
 * It will automatically use the tilemap for collision detection.
 */
class TileMapCollider2D : public Collider2D {
public:
    REQUIRE_COMPONENT(TileMap)
    
    /** Reference to the tilemap (auto-cached on Awake) */
    TileMap* tileMap = nullptr;

    TileMapCollider2D() = default;
    
    void Awake() override;
    
    /** Get bounds - returns full tilemap bounds */
    Bounds2D GetBounds() const override;
    
    /** Check if this tilemap collider overlaps with another collider */
    bool Overlaps(const Collider2D* other) const override;
    
    /** Check if a circle overlaps any wall in the tilemap */
    bool OverlapsCircle(const Vector2& center, f32 radius) const;
    
    /** Check if a box overlaps any wall in the tilemap */
    bool OverlapsBox(const Vector2& center, const Vector2& size) const;
    
    /** Check if a point is inside a wall tile */
    bool PointInWall(f32 x, f32 y) const;
    
    /** Check if position is walkable (not a wall) */
    bool IsWalkable(f32 x, f32 y) const;
    
    /** Try to move from current position by delta, returns allowed position */
    Vector2 ResolveMovement(const Vector2& position, const Vector2& delta, f32 radius) const;
};

} // namespace Engine
