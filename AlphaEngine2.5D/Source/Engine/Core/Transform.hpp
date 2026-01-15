/**
 * @file Transform.hpp
 * @brief Transform component for position, rotation, scale
 */
#pragma once

#include "Component.hpp"
#include "../Math/Vector2.hpp"
#include "../Math/Vector3.hpp"
#include "AEEngine.h"
#include <vector>

namespace Engine {

/**
 * @class Transform
 * @brief Component that holds position, rotation, and scale
 */
class Transform : public Component {
public:
    /** Position in world space (z used for depth sorting) */
    Vector3 position;
    
    /** Rotation in degrees (around z-axis for 2D) */
    f32 rotation = 0;
    
    /** Local scale */
    Vector2 localScale = Vector2(1, 1);
    
    /** Parent transform (nullptr if root) */
    Transform* parent = nullptr;
    
    Transform();

    // Movement methods
    
    /** Translate by delta in world space */
    void Translate(const Vector2& delta);
    
    void Translate(const Vector3& delta);
    
    void Translate(f32 x, f32 y);
    
    /** Rotate by degrees */
    void Rotate(f32 degrees);
    
    /** Look at target position */
    void LookAt(const Vector2& target);

    // Child management
    
    /** Set parent transform */
    void SetParent(Transform* newParent);
    
    /** Get number of children */
    int GetChildCount() const;
    
    /** Get child by index */
    Transform* GetChild(int index) const;

    // Matrix generation for rendering
    
    /** Generate transformation matrix for Alpha Engine */
    AEMtx33 GetMatrix() const;
    
    /** Get world position (including parent transforms) */
    Vector3 GetWorldPosition() const;

private:
    std::vector<Transform*> children;
};

} // namespace Engine
