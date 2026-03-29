/**
 * @file TileMapCollider2D.cpp
 * @brief Implementation of TileMapCollider2D
 */
#include "TileMapCollider2D.hpp"
#include "CircleCollider2D.hpp"
#include "BoxCollider2D.hpp"
#include "../Core/GameObject.hpp"

namespace Engine {

/*!
 * \brief [AUTO] Awake
 */
void TileMapCollider2D::Awake() {
    // Set layer to TileMap so it's filtered out of normal collision checks
    layer = CollisionLayer::TileMap;
    
    // Call base to register with Physics2D
    Collider2D::Awake();
    
    // Get tilemap from same GameObject
    if (gameObject) {
        tileMap = gameObject->GetComponent<TileMap>();
    }
}

/*!
 * \brief [AUTO] GetBounds
 * \return 
 */
Bounds2D TileMapCollider2D::GetBounds() const {
    Bounds2D bounds;
    if (tileMap) {
        bounds.min = Vector2(0, 0);
        bounds.max = Vector2(static_cast<f32>(tileMap->width), static_cast<f32>(tileMap->height));
    }
    return bounds;
}

/*!
 * \brief [AUTO] PointInWall
 * \param x [in]
 * \param y [in]
 * \return 
 */
bool TileMapCollider2D::PointInWall(f32 x, f32 y) const {
    if (!tileMap) return false;
    int tileX = static_cast<int>(x);
    int tileY = static_cast<int>(y);
    return tileMap->IsWall(tileX, tileY);
}

/*!
 * \brief [AUTO] IsWalkable
 * \param x [in]
 * \param y [in]
 * \return 
 */
bool TileMapCollider2D::IsWalkable(f32 x, f32 y) const {
    if (!tileMap) return true;
    return tileMap->IsWalkable(x, y);
}

/*!
 * \brief [AUTO] OverlapsCircle
 * \param center [in]
 * \param radius [in]
 * \return 
 */
bool TileMapCollider2D::OverlapsCircle(const Vector2& center, f32 radius) const {
    if (!tileMap) return false;
    
    // Check all tiles that the circle could touch
    int minTileX = static_cast<int>(center.x - radius);
    int maxTileX = static_cast<int>(center.x + radius);
    int minTileY = static_cast<int>(center.y - radius);
    int maxTileY = static_cast<int>(center.y + radius);
    
    for (int ty = minTileY; ty <= maxTileY; ++ty) {
        for (int tx = minTileX; tx <= maxTileX; ++tx) {
            if (!tileMap->IsWall(tx, ty)) continue;
            
            // Find closest point on tile to circle center
            f32 closestX = center.x;
            f32 closestY = center.y;
            
            // Clamp to tile bounds
            if (closestX < tx) closestX = static_cast<f32>(tx);
            else if (closestX > tx + 1) closestX = static_cast<f32>(tx + 1);
            
            if (closestY < ty) closestY = static_cast<f32>(ty);
            else if (closestY > ty + 1) closestY = static_cast<f32>(ty + 1);
            
            // Check distance
            f32 dx = center.x - closestX;
            f32 dy = center.y - closestY;
            if (dx * dx + dy * dy <= radius * radius) {
                return true;
            }
        }
    }
    return false;
}

/*!
 * \brief [AUTO] OverlapsBox
 * \param center [in]
 * \param size [in]
 * \return 
 */
bool TileMapCollider2D::OverlapsBox(const Vector2& center, const Vector2& size) const {
    if (!tileMap) return false;
    
    Vector2 halfSize = size * 0.5f;
    Vector2 min = center - halfSize;
    Vector2 max = center + halfSize;
    
    int minTileX = static_cast<int>(min.x);
    int maxTileX = static_cast<int>(max.x);
    int minTileY = static_cast<int>(min.y);
    int maxTileY = static_cast<int>(max.y);
    
    for (int ty = minTileY; ty <= maxTileY; ++ty) {
        for (int tx = minTileX; tx <= maxTileX; ++tx) {
            if (tileMap->IsWall(tx, ty)) {
                return true;
            }
        }
    }
    return false;
}

/*!
 * \brief [AUTO] Overlaps
 * \param other [in]
 * \return 
 */
bool TileMapCollider2D::Overlaps(const Collider2D* other) const {
    if (!other || !tileMap) return false;
    
    // Height check
    if (!HeightOverlaps(other)) return false;
    
    // Check based on other collider type
    const CircleCollider2D* circle = dynamic_cast<const CircleCollider2D*>(other);
    if (circle) {
        return OverlapsCircle(circle->GetCenter(), circle->GetScaledRadius());
    }
    
    const BoxCollider2D* box = dynamic_cast<const BoxCollider2D*>(other);
    if (box) {
        Bounds2D boxBounds = box->GetBounds();
        return OverlapsBox(boxBounds.Center(), boxBounds.Size());
    }
    
    // Fallback: check corners of other's bounds
    Bounds2D otherBounds = other->GetBounds();
    return PointInWall(otherBounds.min.x, otherBounds.min.y) ||
           PointInWall(otherBounds.max.x, otherBounds.min.y) ||
           PointInWall(otherBounds.min.x, otherBounds.max.y) ||
           PointInWall(otherBounds.max.x, otherBounds.max.y);
}

/*!
 * \brief [AUTO] ResolveMovement
 * \param position [in]
 * \param delta [in]
 * \param radius [in]
 * \return 
 */
Vector2 TileMapCollider2D::ResolveMovement(const Vector2& position, const Vector2& delta, f32 radius) const {
    if (!tileMap) return position + delta;
    
    Vector2 newPos = position;
    
    // Try X movement
    f32 nextX = position.x + delta.x;
    if (IsWalkable(nextX - radius, position.y - radius) &&
        IsWalkable(nextX + radius, position.y - radius) &&
        IsWalkable(nextX - radius, position.y + radius) &&
        IsWalkable(nextX + radius, position.y + radius)) {
        newPos.x = nextX;
    }
    
    // Try Y movement (using updated X)
    f32 nextY = position.y + delta.y;
    if (IsWalkable(newPos.x - radius, nextY - radius) &&
        IsWalkable(newPos.x + radius, nextY - radius) &&
        IsWalkable(newPos.x - radius, nextY + radius) &&
        IsWalkable(newPos.x + radius, nextY + radius)) {
        newPos.y = nextY;
    }
    
    return newPos;
}

} // namespace Engine
