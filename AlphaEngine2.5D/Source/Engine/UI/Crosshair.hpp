/**
 * @file Crosshair.hpp
 * @brief UI crosshair element that changes appearance when highlighting interactable objects
 */
#pragma once

#include "Image.hpp"
#include "../Graphics/Texture2D.hpp"

namespace UI {

/**
 * @class Crosshair
 * @brief Specialized Image that swaps textures based on highlight state
 * 
 * Centers itself on screen and displays a default texture normally,
 * switching to a highlight texture when targeting an interactable object.
 */
class Crosshair : public Image {
public:
    Crosshair();
    virtual ~Crosshair() = default;
    
    /**
     * @brief Set the default (non-highlighted) texture
     * @param texture Texture to display when not targeting anything
     */
    void SetDefaultTexture(Engine::Texture2D* texture);
    
    /**
     * @brief Set the highlighted texture
     * @param texture Texture to display when targeting an interactable
     */
    void SetHighlightTexture(Engine::Texture2D* texture);
    
    /**
     * @brief Set whether the crosshair is in highlighted state
     * @param highlighted True if targeting an interactable
     */
    void SetHighlighted(bool highlighted);
    
    /**
     * @brief Check if crosshair is highlighted
     */
    bool IsHighlighted() const { return m_highlighted; }
    
private:
    Engine::Texture2D* m_defaultTexture = nullptr;
    Engine::Texture2D* m_highlightTexture = nullptr;
    bool m_highlighted = false;
    
    void UpdateTexture();
};

} // namespace UI
