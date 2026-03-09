#include "Hands67.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/AssetManager.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/UI/Core/UIManager.hpp"

namespace Game {

    void Hands67::Awake() {
        if (!UI::UIManager::GetInstance()) return;

        // Create canvas
        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        m_canvas->SetSortOrder(50); // Below game HUD, below pause menu
        m_canvas->SetReferenceResolution(1600.f, 900.f);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);

        // Create Panel for organization
        m_panel = m_canvas->CreateChild<UI::Panel>();
        m_panel->name = "Hands67Panel";
        m_panel->SetAnchoredPosition({ 0.5f, 0.5f });
        m_panel->SetBackgroundColor(Engine::Color(0, 0, 0, 0)); // Transparent

        // Create SpriteSheetImage
        m_handsImage = m_panel->CreateChild<UI::SpriteSheetImage>();
        m_handsImage->name = "Hands67Image";
        
        // Full screen or centered large
        m_handsImage->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
        m_handsImage->SetPivot({ 0.5f, 0.5f });
        m_handsImage->SetAnchoredPosition({ 0.0f, 0.0f });
        m_handsImage->SetSizeDelta({ 1600.0f, 900.0f });
        
        // Load sprite sheet
        auto* sheet = Engine::AssetManager::GetSpriteSheet("Assets/67.png");
        if (sheet) {
            m_handsImage->SetSpriteSheet(sheet);
            m_handsImage->Play("Show");
        }

        m_panel->visible = false;
        m_isVisible = false;
    }

    void Hands67::Update() {
        // Toggle logic: If holding 6 and pressing 7, or holding 7 and pressing 6, or hitting both
        bool currentSix = Engine::Input::GetKey(Engine::KeyCode::Alpha6);
        bool currentSeven = Engine::Input::GetKey(Engine::KeyCode::Alpha7);
        
        // Detect a new overlap where both are held this frame, but weren't both held last frame
        bool bothPressedNow = (currentSix && currentSeven);
        bool bothPressedBefore = (m_sixPressed && m_sevenPressed);

        if (bothPressedNow && !bothPressedBefore) {
            m_isVisible = !m_isVisible;
            if (m_panel) {
                m_panel->visible = m_isVisible;
            }
        }
        
        m_sixPressed = currentSix;
        m_sevenPressed = currentSeven;
    }

} // namespace Game
