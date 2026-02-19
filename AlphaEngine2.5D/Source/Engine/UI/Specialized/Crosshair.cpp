/**
 * @file Crosshair.cpp
 * @brief Implementation of Crosshair UI element
 */
#include "Engine/UI/Specialized/Crosshair.hpp"

namespace UI {

Crosshair::Crosshair() {
    // Center the crosshair on screen by default
    SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
    SetPivot({ 0.5f, 0.5f });
    SetAnchoredPosition({ 0.0f, 0.0f });
    SetSizeDelta({ 32.0f, 32.0f }); // Default size
    
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
        SetTexture(m_highlightTexture);
    } else {
        SetTexture(m_defaultTexture);
    }
}

} // namespace UI
