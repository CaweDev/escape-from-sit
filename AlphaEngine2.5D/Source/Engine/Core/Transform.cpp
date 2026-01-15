/**
 * @file Transform.cpp
 * @brief Implementation of Transform class
 */
#include "Transform.hpp"
#include <algorithm>
#include <cmath>

namespace Engine {

Transform::Transform() : position(0, 0, 0), rotation(0), localScale(1, 1), parent(nullptr) {}

void Transform::Translate(const Vector2& delta) {
    position.x += delta.x;
    position.y += delta.y;
}

void Transform::Translate(const Vector3& delta) {
    position += delta;
}

void Transform::Translate(f32 x, f32 y) {
    position.x += x;
    position.y += y;
}

void Transform::Rotate(f32 degrees) {
    rotation += degrees;
    // Keep rotation in 0-360 range
    while (rotation >= 360.0f) rotation -= 360.0f;
    while (rotation < 0.0f) rotation += 360.0f;
}

void Transform::LookAt(const Vector2& target) {
    f32 dx = target.x - position.x;
    f32 dy = target.y - position.y;
    rotation = std::atan2(dy, dx) * (180.0f / 3.14159265f);
}

void Transform::SetParent(Transform* newParent) {
    // Remove from old parent
    if (parent) {
        auto& siblings = parent->children;
        siblings.erase(
            std::remove(siblings.begin(), siblings.end(), this),
            siblings.end()
        );
    }
    
    parent = newParent;
    
    // Add to new parent
    if (parent) {
        parent->children.push_back(this);
    }
}

int Transform::GetChildCount() const {
    return static_cast<int>(children.size());
}

Transform* Transform::GetChild(int index) const {
    if (index >= 0 && index < static_cast<int>(children.size())) {
        return children[index];
    }
    return nullptr;
}

AEMtx33 Transform::GetMatrix() const {
    AEMtx33 scale, rot, trans, result;
    
    AEMtx33Scale(&scale, localScale.x, localScale.y);
    AEMtx33RotDeg(&rot, rotation);
    AEMtx33Trans(&trans, position.x, position.y);
    
    // Combine: Scale -> Rotate -> Translate
    AEMtx33Concat(&result, &rot, &scale);
    AEMtx33Concat(&result, &trans, &result);
    
    // Apply parent transform if exists
    if (parent) {
        AEMtx33 parentMat = parent->GetMatrix();
        AEMtx33Concat(&result, &parentMat, &result);
    }
    
    return result;
}

Vector3 Transform::GetWorldPosition() const {
    if (parent) {
        Vector3 parentPos = parent->GetWorldPosition();
        return Vector3(
            parentPos.x + position.x,
            parentPos.y + position.y,
            parentPos.z + position.z
        );
    }
    return position;
}

} // namespace Engine
