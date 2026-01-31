/**
 * @file Crosshair.cpp
 * @brief Implementation of Crosshair UI element
 */
#include "Crosshair.hpp"

namespace UI {

Crosshair::Crosshair() {
    // Center the crosshair on screen by default
    rectTransform.anchorMin = { 0.5f, 0.5f };
    rectTransform.anchorMax = { 0.5f, 0.5f };
    rectTransform.pivot = { 0.5f, 0.5f };
    rectTransform.anchoredPosition = { 0.0f, 0.0f };
    rectTransform.sizeDelta = { 32.0f, 32.0f }; // Default size
    
    // Crosshair shouldn't block raycasts
    raycastTarget = false;

    m_renderOrder = 100000;
    
    name = "Crosshair";
}

void Crosshair::SetDefaultTexture(Engine::Texture2D* texture) {
    m_defaultTexture = texture;
    UpdateTexture();
}

void Crosshair::SetHighlightTexture(Engine::Texture2D* texture) {
    m_highlightTexture = texture;
    UpdateTexture();
}

void Crosshair::SetHighlighted(bool highlighted) {
    if (m_highlighted != highlighted) {
        m_highlighted = highlighted;
        UpdateTexture();
    }
}

void Crosshair::UpdateTexture() {
    if (m_highlighted && m_highlightTexture) {
        SetSprite(m_highlightTexture);
    } else {
        SetSprite(m_defaultTexture);
    }
}

} // namespace UI
