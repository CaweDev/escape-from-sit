/**
 * @file Interactable.hpp
 * @brief Component that marks a GameObject as interactable
 */
#pragma once
#include "AEEngine.h"
#include "Engine/Core/Component.hpp"
#include <string>
#include <functional>

namespace Engine {

/**
 * @enum HitShape
 * @brief Shape used for interaction hit detection
 */
enum class InteractableHitShape {
    Rectangle,  // Uses full bounding box
    Ellipse     // Uses ellipse inscribed in bounding box
};

/**
 * @class Interactable
 * @brief Marks a GameObject as interactable when player looks at it
 * 
 * Attach this component to any GameObject with a billboard SpriteRenderer
 * to make it interactable. The InteractionController will automatically
 * detect it when the player looks at it.
 */
class Interactable : public Component {
public:
    /** Text shown in the action prompt (e.g., "Open", "Pick up") */
    std::string actionText = "Interact";
    
    /** Maximum distance from camera for interaction (world units) */
    f32 maxDistance = 5.0f;
    
    /** Shape used for hit detection */
    InteractableHitShape hitShape = InteractableHitShape::Rectangle;
    
    /** Callback when player interacts (optional, for C++ usage) */
    std::function<void()> onInteract;
    
    Interactable() = default;
    virtual ~Interactable() = default;
    
    /**
     * @brief Called by InteractionController when player confirms interaction
     */
    void Interact();
};

} // namespace Engine
