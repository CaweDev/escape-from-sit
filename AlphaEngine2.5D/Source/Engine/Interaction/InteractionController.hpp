/**
 * @file InteractionController.hpp
 * @brief Component that handles player interaction with objects
 */
#pragma once

#include "Engine/Core/Component.hpp"
#include "Engine/Input/KeyCode.hpp"
#include "Interactable.hpp"

// Forward declarations
namespace UI {
    class Canvas;
    class Crosshair;
    class InteractionPrompt;
}

namespace Engine {

class SpriteRenderer;
class Texture2D;

/**
 * @class InteractionController
 * @brief Player component that detects and interacts with Interactable objects
 * 
 * Attach this component to the player's camera GameObject. It will:
 * - Only run when Camera::main->gameObject matches this->gameObject
 * - Raycast from screen center to find interactable objects
 * - Update crosshair and prompt UI
 * - Highlight the targeted sprite
 * - Handle interaction input
 */
class InteractionController : public Component {
public:
    /** Key to trigger interaction */
    KeyCode interactKey = KeyCode::E;
    
    /** Default crosshair texture (set by user) */
    Texture2D* defaultCrosshairTexture = nullptr;
    
    /** Highlighted crosshair texture (set by user) */
    Texture2D* highlightCrosshairTexture = nullptr;
    
    InteractionController() = default;
    virtual ~InteractionController() = default;
    
    /**
     * @brief Get the currently targeted interactable (nullptr if none)
     */
    Interactable* GetCurrentTarget() const { return m_currentTarget; }
    
    // Lifecycle
    void Initialize() override;
    void Update() override;
    void OnDestroy() override;
    
private:
    Interactable* m_currentTarget = nullptr;
    SpriteRenderer* m_currentTargetRenderer = nullptr;
    
    // UI elements (created and owned by this controller)
    UI::Canvas* m_canvas = nullptr;
    UI::Crosshair* m_crosshair = nullptr;
    UI::InteractionPrompt* m_prompt = nullptr;
    
    /**
     * @brief Find the interactable at screen center
     * @return The closest interactable in view, or nullptr
     */
    Interactable* FindTargetAtScreenCenter();
    
    /**
     * @brief Update target and associated UI/highlighting
     * @param target The new target (or nullptr to clear)
     */
    void SetTarget(Interactable* target);
    
    /**
     * @brief Check if this controller should be active
     * @return True if Camera::main is on this gameObject
     */
    bool IsActiveCamera() const;
};

} // namespace Engine
