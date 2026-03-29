/**
 * @file Collider2D.cpp
 * @brief Implementation of Collider2D base class
 */
#include "Collider2D.hpp"
#include "Physics2D.hpp"
#include "../Core/Transform.hpp"

namespace Engine {

/*!
 * \brief [AUTO] Awake
 */
void Collider2D::Awake() {
    Physics2D::RegisterCollider(this);
}

/*!
 * \brief [AUTO] OnDestroy
 */
void Collider2D::OnDestroy() {
    Physics2D::UnregisterCollider(this);
}

/*!
 * \brief [AUTO] GetCenter
 * \return 
 */
Vector2 Collider2D::GetCenter() const {
    if (!transform) return offset;
    return Vector2(transform->position.x, transform->position.y) + offset;
}

/*!
 * \brief [AUTO] Overlaps
 * \param other [in]
 * \return 
 */
bool Collider2D::Overlaps(const Collider2D* other) const {
    if (!other) return false;
    
    // First check height overlap for 2.5D
    if (!HeightOverlaps(other)) return false;
    
    // Then check 2D bounds overlap
    return GetBounds().Overlaps(other->GetBounds());
}

/*!
 * \brief [AUTO] HeightOverlaps
 * \param other [in]
 * \return 
 */
bool Collider2D::HeightOverlaps(const Collider2D* other) const {
    if (!other) return false;
    return !(maxHeight <= other->minHeight || minHeight >= other->maxHeight);
}

/*!
 * \brief [AUTO] HeightContains
 * \param height [in]
 * \param objectHeight [in]
 * \return 
 */
bool Collider2D::HeightContains(f32 height, f32 objectHeight) const {
    f32 objectTop = height + objectHeight;
    return !(objectTop <= minHeight || height >= maxHeight);
}

} // namespace Engine
