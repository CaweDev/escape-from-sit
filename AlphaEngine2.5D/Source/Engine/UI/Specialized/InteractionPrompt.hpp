/**
 * @file InteractionPrompt.hpp
 * @brief UI element that displays interaction prompt text (e.g., "[E] Open")
 */
#pragma once

#include "Engine/UI/Core/UIElement.hpp"
#include "Engine/UI/Components/Text.hpp"
#include <string>

namespace UI {

/**
 * @class InteractionPrompt
 * @brief Displays an interaction prompt below the crosshair
 * 
 * Shows text like "[E] Open" when the player is looking at an
 * interactable object. Uses the UI hierarchy system with Text as a child.
 */
class InteractionPrompt : public UIElement {
public:
    InteractionPrompt();
    virtual ~InteractionPrompt() = default;
    
    /**
     * @brief Show the prompt with the given action text
     * @param actionText The action description (e.g., "Open", "Pick up")
     */
    void Show(const std::string& actionText);
    
    /**
     * @brief Hide the prompt
     */
    void Hide();
    
    /**
     * @brief Check if the prompt is currently visible
     */
    bool IsShowing() const { return visible; }
    
    /**
     * @brief Set the key text prefix (e.g., "[E]")
     * @param keyText The key indicator to show before action text
     */
    void SetKeyText(const std::string& keyText);
    
    /**
     * @brief Set the text color
     */
    void SetTextColor(const Engine::Color& color);
    
    /**
     * @brief Set the font size
     */
    void SetFontSize(f32 size);
    
private:
    Text* m_text = nullptr;
    std::string m_keyText = "[E]";
    std::string m_currentActionText;
    
    void UpdateText();
};

} // namespace UI
