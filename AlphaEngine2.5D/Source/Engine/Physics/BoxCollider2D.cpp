/**
 * @file BoxCollider2D.cpp
 * @brief Implementation of BoxCollider2D
 */
#include "BoxCollider2D.hpp"
#include "../Core/Transform.hpp"

namespace Engine {



Bounds2D BoxCollider2D::GetBounds() const {
    Vector2 center = GetCenter();
    Vector2 halfSize = size * 0.5f;
    
    // Apply transform scale if available
    if (transform) {
        halfSize.x *= transform->localScale.x;
        halfSize.y *= transform->localScale.y;
    }
    
    Bounds2D bounds;
    bounds.min = center - halfSize;
    bounds.max = center + halfSize;
    return bounds;
}



bool BoxCollider2D::ContainsPoint(const Vector2& point) const {
    Bounds2D bounds = GetBounds();
    return point.x >= bounds.min.x && point.x <= bounds.max.x &&
           point.y >= bounds.min.y && point.y <= bounds.max.y;
}

} // namespace Engine
