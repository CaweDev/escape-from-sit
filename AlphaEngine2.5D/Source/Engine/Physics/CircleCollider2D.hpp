/**
 * @file CircleCollider2D.hpp
 * @brief Circle collider component
 */
#pragma once

#include "Collider2D.hpp"

namespace Engine {

/**
 * @class CircleCollider2D
 * @brief Circular collider for round objects
 */
class CircleCollider2D : public Collider2D {
public:
    /** Radius of the circle */
    f32 radius = 0.5f;

    CircleCollider2D() = default;
    
    /** Get the axis-aligned bounding box (for broad-phase) */
    Bounds2D GetBounds() const override;
    
    /** Check if this circle overlaps with another collider */
    bool Overlaps(const Collider2D* other) const override;
    
    /** Check if a point is inside this circle */
    bool ContainsPoint(const Vector2& point) const;
    
    /** Get the scaled radius (accounting for transform scale) */
    f32 GetScaledRadius() const;
};

} // namespace Engine
