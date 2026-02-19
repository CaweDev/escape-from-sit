/**
 * @file InteractionPrompt.cpp
 * @brief Implementation of InteractionPrompt UI element
 */
#include "Engine/UI/Specialized/InteractionPrompt.hpp"

namespace UI {

InteractionPrompt::InteractionPrompt() {
    // Position below screen center (where crosshair is)
    SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
    SetPivot({ 0.5f, 1.0f });  // Pivot at top center
    SetAnchoredPosition({ 0.0f, -40.0f });  // Below crosshair
    SetSizeDelta({ 200.0f, 30.0f });
    
    // Not a raycast target
    raycastTarget = false;
    
    // Create text child using hierarchy
    m_text = CreateChild<Text>();
    m_text->SetAnchor(TextAnchor::MiddleCenter);
    m_text->SetColor(Engine::Color::white());
    m_text->SetFontSize(1.0f);
    
    // Fill parent
    m_text->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
    m_text->SetSizeDelta({ 0.0f, 0.0f });
    
    // Start hidden
    visible = false;
    
    name = "InteractionPrompt";
}

void InteractionPrompt::Show(const std::string& actionText) {
    m_currentActionText = actionText;
    UpdateText();
    visible = true;
}

void InteractionPrompt::Hide() {
    visible = false;
}

void InteractionPrompt::SetKeyText(const std::string& keyText) {
    m_keyText = keyText;
    UpdateText();
}

void InteractionPrompt::SetTextColor(const Engine::Color& color) {
    if (m_text) {
        m_text->SetColor(color);
    }
}

void InteractionPrompt::SetFontSize(f32 size) {
    if (m_text) {
        m_text->SetFontSize(size);
    }
}

void InteractionPrompt::UpdateText() {
    if (m_text) {
        m_text->SetText(m_keyText + " " + m_currentActionText);
    }
}

} // namespace UI
