#include "ControlsMenu.hpp"
#include <iostream>

namespace Game
{
    namespace
    {
        const Engine::Color kOverlayColor   { 0.0f,  0.0f,  0.0f,  0.85f };
        const Engine::Color kTitleColor     { 0.8f,  0.15f, 0.10f, 1.0f };
        
        const Engine::Color kBtnNormal      { 0.75f, 0.08f, 0.08f, 1.0f };
        const Engine::Color kBtnHighlighted { 0.95f, 0.08f, 0.08f, 1.0f };
        const Engine::Color kBtnPressed     { 0.30f, 0.10f, 0.10f, 1.0f };

        const Engine::Color kTextNormal     { 0.9f,  0.9f,  0.9f,  1.0f };
        const Engine::Color kTextSelected   { 1.0f,  1.0f,  1.0f,  1.0f };

        constexpr const char* kCursorUnlockId = "ControlsMenu";
        constexpr const char* kPauseId = "ControlsMenu";
    }

    ControlsMenu::~ControlsMenu()
    {
    }

    void ControlsMenu::Awake()
    {
        BuildUI();
    }

    void ControlsMenu::Update()
    {
        if (!m_isVisible) return;

        // Close on Escape
        if (Engine::Input::GetKeyDown(Engine::KeyCode::Escape))
        {
            Hide();
            return;
        }

        // Mouse hover detection for back button
        Engine::Vector2 mousePos = Engine::Input::GetMousePosition();
        auto canvasPos = m_canvas->ScreenToCanvasPosition(
            static_cast<s32>(mousePos.x), static_cast<s32>(mousePos.y));
        f32 worldX = canvasPos.x, worldY = canvasPos.y;

        bool isHovering = false;
        if (m_backBtn)
        {
            UI::Rect btnRect = m_backBtn->GetWorldRect();
            if (worldX >= btnRect.x && worldX <= btnRect.x + btnRect.width &&
                worldY >= btnRect.y && worldY <= btnRect.y + btnRect.height)
            {
                isHovering = true;
            }
        }

        if (isHovering != m_isHovered)
        {
            m_isHovered = isHovering;
            if (m_isHovered) {
                Engine::SoundManager::PlaySFX("ui_hover", 0.7f);
            }
            UpdateButtonVisuals();
        }

        // Mouse click logic
        if (Engine::Input::GetMouseButtonDown(0) && m_isHovered)
        {
            m_isPressed = true;
            UpdateButtonVisuals();
        }

        if (Engine::Input::GetMouseButtonUp(0))
        {
            if (m_isPressed && m_isHovered)
            {
                OnBackClicked();
            }
            m_isPressed = false;
            UpdateButtonVisuals();
        }
    }

    void ControlsMenu::OnDestroy()
    {
        Engine::Input::UnregisterCursorUnlock(kCursorUnlockId);
        Engine::Application::UnregisterPause(kPauseId);
        m_isVisible = false;

        if (m_canvas)
        {
            UI::UIManager::GetInstance()->DestroyCanvas(m_canvas);
            m_canvas = nullptr;
        }
    }

    void ControlsMenu::Show()
    {
        if (m_isVisible) return;
        m_isVisible = true;

        Engine::Application::RegisterPause(kPauseId);
        Engine::Input::RegisterCursorUnlock(kCursorUnlockId);
        if (m_overlay) m_overlay->visible = true;
        Engine::SoundManager::PlaySFX("ui_toggle", 0.8f);
    }

    void ControlsMenu::Hide()
    {
        if (!m_isVisible) return;
        m_isVisible = false;

        Engine::Application::UnregisterPause(kPauseId);
        Engine::Input::UnregisterCursorUnlock(kCursorUnlockId);
        if (m_overlay) m_overlay->visible = false;
        Engine::SoundManager::PlaySFX("ui_toggle", 0.8f);
    }

    void ControlsMenu::Toggle()
    {
        if (m_isVisible) Hide();
        else             Show();
    }

    void ControlsMenu::BuildUI()
    {
        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        m_canvas->SetReferenceResolution(1280.0f, 720.0f);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);
        m_canvas->SetSortOrder(600); // Above Main Menu and Pause Menu

        // Full screen overlay
        m_overlay = m_canvas->CreateChild<UI::Panel>();
        m_overlay->name = "ControlsOverlay";
        m_overlay->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_overlay->SetAnchoredPosition({ 0.0f, 0.0f });
        m_overlay->SetSizeDelta({ 0.0f, 0.0f });
        m_overlay->SetBackgroundColor(kOverlayColor);
        m_overlay->visible = false;

        // Background Panel from UI Kit
        m_panelImage = m_overlay->CreateChild<UI::Image>();
        m_panelImage->name = "ControlsPanelBase";
        m_panelImage->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
        m_panelImage->SetPivot({ 0.5f, 0.5f });
        m_panelImage->SetAnchoredPosition({ 0.0f, 0.0f });
        m_panelImage->SetSizeDelta({ 1120.0f, 760.0f }); // Increased menu size
        m_panelImage->SetTexture(Engine::AssetManager::GetTexture("Assets/UI/UI Kit/Panel/panel-000.png"));
		m_panelImage->SetImageType(UI::ImageType::Sliced);
		m_panelImage->SetPixelsPerUnitMultiplier(4.0f); // Make the 9-slice borders thicker
		m_panelImage->SetColor({ 1.0f, 1.0f, 1.0f, 1.f }); // Slightly transparent to show overlay tint

        // Controls Image
        m_controlsImage = m_panelImage->CreateChild<UI::Image>();
        m_controlsImage->name = "ControlsContent";
        m_controlsImage->SetAnchors({ 0.5f, 0.55f }, { 0.5f, 0.55f });
        m_controlsImage->SetPivot({ 0.5f, 0.5f });
        m_controlsImage->SetAnchoredPosition({ 0.0f, -20.0f });
        m_controlsImage->SetSizeDelta({ 980.0f, 620.0f });
        m_controlsImage->SetTexture(Engine::AssetManager::GetTexture("Assets/UI/Controls.png"));

        // Back Button
        m_backBtn = m_panelImage->CreateChild<UI::Button>();
        m_backBtn->name = "Close";
        m_backBtn->SetAnchors({ 0.5f, 0.0f }, { 0.5f, 0.0f });
        m_backBtn->SetPivot({ 0.5f, 0.5f });
        m_backBtn->SetAnchoredPosition({ 0.0f, 20.f });
        m_backBtn->SetSizeDelta({ 240.0f, 50.0f });
        m_backBtn->SetBackgroundSprite(Engine::AssetManager::GetTexture("Assets/UI/UI Kit/Panel/panel-027.png"));
        m_backBtn->GetBackground()->SetImageType(UI::ImageType::Sliced);
        m_backBtn->GetBackground()->SetPixelsPerUnitMultiplier(1.0f); // Make the 9-slice borders thicker
        
        UI::ColorBlock btnColors;
        btnColors.normalColor      = kBtnNormal;
        btnColors.highlightedColor = kBtnHighlighted;
        btnColors.pressedColor     = kBtnPressed;
        
        m_backBtn->SetColors(btnColors);
        m_backBtn->SetBackgroundColor(kBtnNormal);
        m_backBtn->SetText("Back");
        m_backBtn->onClick = [this]() { OnBackClicked(); };
        
        if (auto* label = m_backBtn->GetLabel())
        {
            label->SetFontSize(2.0f);
            label->SetColor(kTextNormal);
        }
    }

    void ControlsMenu::UpdateButtonVisuals()
    {
        if (!m_backBtn) return;
        
        if (m_isPressed)
        {
            m_backBtn->SetBackgroundColor(kBtnPressed);
            if (auto* lbl = m_backBtn->GetLabel()) lbl->SetColor(kTextSelected);
        }
        else if (m_isHovered)
        {
            m_backBtn->SetBackgroundColor(kBtnHighlighted);
            if (auto* lbl = m_backBtn->GetLabel()) lbl->SetColor(kTextSelected);
        }
        else
        {
            m_backBtn->SetBackgroundColor(kBtnNormal);
            if (auto* lbl = m_backBtn->GetLabel()) lbl->SetColor(kTextNormal);
        }
    }

    void ControlsMenu::OnBackClicked()
    {
        Engine::SoundManager::PlaySFX("ui_click");
        Hide();
    }
}
