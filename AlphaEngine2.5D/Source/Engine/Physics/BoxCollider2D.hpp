/**
 * @file BoxCollider2D.hpp
 * @brief Axis-aligned box collider component
 */
#pragma once

#include "Collider2D.hpp"

namespace Engine {

/**
 * @class BoxCollider2D
 * @brief Axis-aligned rectangular collider
 */
class BoxCollider2D : public Collider2D {
public:
    /** Size of the box (width, height) */
    Vector2 size = Vector2(1.0f, 1.0f);

    BoxCollider2D() = default;
    
    /** Get the axis-aligned bounding box */
    Bounds2D GetBounds() const override;
    
    /** Check if a point is inside this box collider */
    bool ContainsPoint(const Vector2& point) const;
};

} // namespace Engine
