/**
 * @file CircleCollider2D.cpp
 * @brief Implementation of CircleCollider2D
 */
#include "CircleCollider2D.hpp"
#include "BoxCollider2D.hpp"
#include "../Core/Transform.hpp"
#include "../Math/Mathf.hpp"

namespace Engine {

/*!
 * \brief [AUTO] GetScaledRadius
 * \return 
 */
f32 CircleCollider2D::GetScaledRadius() const {
    if (!transform) return radius;
    // Use the larger scale axis for uniform circle scaling
    return radius * Mathf::Max(transform->localScale.x, transform->localScale.y);
}

/*!
 * \brief [AUTO] GetBounds
 * \return 
 */
Bounds2D CircleCollider2D::GetBounds() const {
    Vector2 center = GetCenter();
    f32 r = GetScaledRadius();
    
    Bounds2D bounds;
    bounds.min = center - Vector2(r, r);
    bounds.max = center + Vector2(r, r);
    return bounds;
}

/*!
 * \brief [AUTO] ContainsPoint
 * \param point [in]
 * \return 
 */
bool CircleCollider2D::ContainsPoint(const Vector2& point) const {
    Vector2 center = GetCenter();
    f32 r = GetScaledRadius();
    return Vector2::Distance(point, center) <= r;
}

/*!
 * \brief [AUTO] Overlaps
 * \param other [in]
 * \return 
 */
bool CircleCollider2D::Overlaps(const Collider2D* other) const {
    if (!other) return false;
    
    // Check height overlap first
    if (!HeightOverlaps(other)) return false;
    
    // Circle vs Circle
    const CircleCollider2D* otherCircle = dynamic_cast<const CircleCollider2D*>(other);
    if (otherCircle) {
        Vector2 center1 = GetCenter();
        Vector2 center2 = otherCircle->GetCenter();
        f32 r1 = GetScaledRadius();
        f32 r2 = otherCircle->GetScaledRadius();
        return Vector2::Distance(center1, center2) <= (r1 + r2);
    }
    
    // Circle vs Box
    const BoxCollider2D* otherBox = dynamic_cast<const BoxCollider2D*>(other);
    if (otherBox) {
        Vector2 center = GetCenter();
        f32 r = GetScaledRadius();
        Bounds2D boxBounds = otherBox->GetBounds();
        
        // Find closest point on box to circle center
        Vector2 closest;
        closest.x = Mathf::Clamp(center.x, boxBounds.min.x, boxBounds.max.x);
        closest.y = Mathf::Clamp(center.y, boxBounds.min.y, boxBounds.max.y);
        
        return Vector2::Distance(center, closest) <= r;
    }
    
    // Fallback to AABB overlap
    return GetBounds().Overlaps(other->GetBounds());
}

} // namespace Engine
