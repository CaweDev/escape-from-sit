/**
 * @file Interactable.hpp
 * @brief Component that marks a GameObject as interactable
 */
#pragma once

#include "Engine/Core/Component.hpp"
#include "AEEngine.h"
#include <string>
#include <functional>
namespace Engine {

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
