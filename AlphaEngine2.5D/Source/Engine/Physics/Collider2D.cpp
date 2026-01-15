/**
 * @file Collider2D.cpp
 * @brief Implementation of Collider2D base class
 */
#include "Collider2D.hpp"
#include "Physics2D.hpp"
#include "../Core/Transform.hpp"

namespace Engine {

void Collider2D::Awake() {
    Physics2D::RegisterCollider(this);
}

void Collider2D::OnDestroy() {
    Physics2D::UnregisterCollider(this);
}

Vector2 Collider2D::GetCenter() const {
    if (!transform) return offset;
    return Vector2(transform->position.x, transform->position.y) + offset;
}

bool Collider2D::Overlaps(const Collider2D* other) const {
    if (!other) return false;
    
    // First check height overlap for 2.5D
    if (!HeightOverlaps(other)) return false;
    
    // Then check 2D bounds overlap
    return GetBounds().Overlaps(other->GetBounds());
}

bool Collider2D::HeightOverlaps(const Collider2D* other) const {
    if (!other) return false;
    return !(maxHeight <= other->minHeight || minHeight >= other->maxHeight);
}

bool Collider2D::HeightContains(f32 height, f32 objectHeight) const {
    f32 objectTop = height + objectHeight;
    return !(objectTop <= minHeight || height >= maxHeight);
}

} // namespace Engine
