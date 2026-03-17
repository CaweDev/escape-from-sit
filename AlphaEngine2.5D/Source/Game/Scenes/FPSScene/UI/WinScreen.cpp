#include "WinScreen.hpp"

namespace Game
{
    namespace
    {
        const Engine::Color kOverlayColor{ 0.0f, 0.0f, 0.0f, 0.75f };
        const Engine::Color kPanelColor{ 0.0f, 0.0f, 0.0f, 1.0f };
        const Engine::Color kTitleColor = Engine::Color::white();
        const Engine::Color kButtonColor{ 0.25f, 0.5f, 0.25f, 1.0f };
    }

    void WinScreen::Awake()
    {
        BuildUI();
    }

    void WinScreen::OnDestroy()
    {
        if (m_visible)
        {
            Engine::Application::UnregisterPause("WinScreen");
            Engine::Input::UnregisterCursorUnlock("WinScreen");
            m_visible = false;
        }

        if (m_canvas)
        {
            UI::UIManager::GetInstance()->DestroyCanvas(m_canvas);
            m_canvas = nullptr;
            m_overlay = nullptr;
            m_panel = nullptr;
            m_title = nullptr;
            m_button = nullptr;
        }
    }

    void WinScreen::BuildUI()
    {
        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        m_canvas->SetSortOrder(1000);
        m_canvas->SetReferenceResolution(1280.0f, 720.0f);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);

        m_overlay = m_canvas->CreateChild<UI::Panel>();
        m_overlay->name = "WinOverlay";
        m_overlay->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_overlay->SetAnchoredPosition({ 0.0f, 0.0f });
        m_overlay->SetSizeDelta({ 0.0f, 0.0f });
        m_overlay->SetBackgroundColor(kOverlayColor);
        m_overlay->visible = false;

        m_panel = m_overlay->CreateChild<UI::Panel>();
        m_panel->name = "WinPanel";
        m_panel->rectTransform.SetCentered(1000.0f, 560.0f);
        m_panel->SetBackgroundColor(kPanelColor);

        m_title = m_panel->CreateChild<UI::Text>();
        m_title->name = "WinTitle";
        m_title->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        m_title->SetPivot({ 0.5f, 1.0f });
        m_title->SetAnchoredPosition({ 0.0f, -24.0f });
        m_title->SetSizeDelta({ 0.0f, 64.0f });
        m_title->SetText("YOU ESCAPED. WIN!");
        m_title->SetFontSize(3.0f);
        m_title->SetColor(kTitleColor);
        m_title->SetAnchor(UI::TextAnchor::MiddleCenter);

        m_button = m_panel->CreateChild<UI::Button>();
        m_button->name = "WinButton";
        m_button->SetAnchors({ 0.5f, 0.0f }, { 0.5f, 0.0f });
        m_button->SetPivot({ 0.5f, 0.0f });
        m_button->SetAnchoredPosition({ 0.0f, 20.0f });
        m_button->SetSizeDelta({ 240.0f, 56.0f });
        m_button->SetText("Main Menu");
        m_button->SetBackgroundColor(kButtonColor);
        m_button->onClick = [this]() { OnButtonClicked(); };
    }

    void WinScreen::Show()
    {
        if (m_visible || !m_overlay) return;

        m_visible = true;
        m_overlay->visible = true;

        Engine::Application::RegisterPause("WinScreen");
        Engine::Input::RegisterCursorUnlock("WinScreen");
        Engine::SoundManager::PlaySFX("win");
    }

    void WinScreen::Hide()
    {
        if (!m_visible || !m_overlay) return;

        m_visible = false;
        m_overlay->visible = false;

        Engine::Input::UnregisterCursorUnlock("WinScreen");
        Engine::Application::UnregisterPause("WinScreen");
    }

    void WinScreen::OnButtonClicked()
    {
        Engine::SoundManager::PlaySFX("ui_click");
        Hide();
        Engine::SceneManager::LoadWithLoadingScreen("MainMenu");
    }
}