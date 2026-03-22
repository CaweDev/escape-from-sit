#include "LoseScreen.hpp"

namespace Game
{
    namespace
    {
        const Engine::Color kOverlayColor{ 0.0f, 0.0f, 0.0f, 0.75f };
        const Engine::Color kPanelColor{ 0.0f, 0.0f, 0.0f, 1.0f };
        const Engine::Color kTitleColor = Engine::Color::white();
        const Engine::Color kButtonColor{ 0.25f, 0.5f, 0.25f, 1.0f };
    }

    void LoseScreen::Awake()
    {
        BuildUI();
    }

    void LoseScreen::Update()
    {
        if (!m_visible || !m_isFadingIn) return;

        m_fadeTimer += Engine::Time::unscaledDeltaTime;
        float t = (m_fadeDuration > 0.0f) ? (m_fadeTimer / m_fadeDuration) : 1.0f;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        ApplyFade(t);

        if (t >= 1.0f)
        {
            m_isFadingIn = false;
        }
    }

    void LoseScreen::ApplyFade(float t)
    {
        Engine::Color overlay = kOverlayColor;
        overlay.a *= t;
        if (m_overlay) m_overlay->SetBackgroundColor(overlay);

        Engine::Color panel = kPanelColor;
        panel.a *= t;
        if (m_panel) m_panel->SetBackgroundColor(panel);

        Engine::Color title = kTitleColor;
        title.a *= t;
        if (m_title) m_title->SetColor(title);

        Engine::Color btn = kButtonColor;
        btn.a *= t;
        if (m_button) m_button->SetBackgroundColor(btn);
        if (m_restartButton) m_restartButton->SetBackgroundColor(btn);

        if (m_button && m_button->GetLabel())
        {
            Engine::Color c = Engine::Color::white();
            c.a = t;
            m_button->GetLabel()->SetColor(c);
        }

        if (m_restartButton && m_restartButton->GetLabel())
        {
            Engine::Color c = Engine::Color::white();
            c.a = t;
            m_restartButton->GetLabel()->SetColor(c);
        }
    }

    void LoseScreen::OnDestroy()
    {
        if (m_visible)
        {
            Engine::Application::UnregisterPause("LoseScreen");
            Engine::Input::UnregisterCursorUnlock("LoseScreen");
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
            m_restartButton = nullptr;
        }
    }

    void LoseScreen::BuildUI()
    {
        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        m_canvas->SetSortOrder(1000);
        m_canvas->SetReferenceResolution(1280.0f, 720.0f);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);

        m_overlay = m_canvas->CreateChild<UI::Panel>();
        m_overlay->name = "LoseOverlay";
        m_overlay->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_overlay->SetAnchoredPosition({ 0.0f, 0.0f });
        m_overlay->SetSizeDelta({ 0.0f, 0.0f });
        m_overlay->SetBackgroundColor(kOverlayColor);
        m_overlay->visible = false;

        m_panel = m_overlay->CreateChild<UI::Panel>();
        m_panel->name = "LosePanel";
        m_panel->rectTransform.SetCentered(1000.0f, 560.0f);
        m_panel->SetBackgroundSprite(Engine::AssetManager::GetTexture("Assets/UI/UI Kit/Panel/panel-000.png"));
        m_panel->GetBackground()->SetImageType(UI::ImageType::Sliced);
        m_panel->GetBackground()->SetPixelsPerUnitMultiplier(4.0f);
        m_panel->SetBackgroundColor(kPanelColor);

        m_title = m_panel->CreateChild<UI::Text>();
        m_title->name = "LoseTitle";
        m_title->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        m_title->SetPivot({ 0.5f, 1.0f });
        m_title->SetAnchoredPosition({ 0.0f, -55.0f });
        m_title->SetSizeDelta({ 0.0f, 64.0f });
        m_title->SetText("YOU GOT CAUGHT. GAME OVER");
        m_title->SetFontSize(3.0f);
        m_title->SetColor(kTitleColor);
        m_title->SetAnchor(UI::TextAnchor::MiddleCenter);

        m_restartButton = m_panel->CreateChild<UI::Button>();
        m_restartButton->name = "LoseRestartButton";
        m_restartButton->SetAnchors({ 0.5f, 0.0f }, { 0.5f, 0.0f });
        m_restartButton->SetPivot({ 0.5f, 0.0f });
        m_restartButton->SetAnchoredPosition({ 0.0f, 125.0f });
        m_restartButton->SetSizeDelta({ 240.0f, 56.0f });
        m_restartButton->SetText("Restart");
        m_restartButton->SetBackgroundColor(kButtonColor);
        m_restartButton->SetBackgroundSprite(Engine::AssetManager::GetTexture("Assets/UI/UI Kit/Panel/panel-027.png"));
        m_restartButton->GetBackground()->SetImageType(UI::ImageType::Sliced);
        m_restartButton->GetBackground()->SetPixelsPerUnitMultiplier(1.0f);
        m_restartButton->onClick = [this]() { OnRestartButtonClicked(); };

        m_button = m_panel->CreateChild<UI::Button>();
        m_button->name = "LoseButton";
        m_button->SetAnchors({ 0.5f, 0.0f }, { 0.5f, 0.0f });
        m_button->SetPivot({ 0.5f, 0.0f });
        m_button->SetAnchoredPosition({ 0.0f, 55.0f });
        m_button->SetSizeDelta({ 240.0f, 56.0f });
        m_button->SetText("Main Menu");
        m_button->SetBackgroundColor(kButtonColor);
        m_button->SetBackgroundSprite(Engine::AssetManager::GetTexture("Assets/UI/UI Kit/Panel/panel-027.png"));
        m_button->GetBackground()->SetImageType(UI::ImageType::Sliced);
        m_button->GetBackground()->SetPixelsPerUnitMultiplier(1.0f);
        m_button->onClick = [this]() { OnButtonClicked(); };

        ApplyFade(1.0f);
    }

    void LoseScreen::Show()
    {
        if (m_visible || !m_overlay) return;

        m_visible = true;
        m_overlay->visible = true;

        m_fadeTimer = 0.0f;
        m_isFadingIn = true;
        ApplyFade(0.0f);

        Engine::Application::RegisterPause("LoseScreen");
        Engine::Input::RegisterCursorUnlock("LoseScreen");
        Engine::SoundManager::PlaySFX("lose");
    }

    void LoseScreen::Hide()
    {
        if (!m_visible || !m_overlay) return;

        m_visible = false;
        m_overlay->visible = false;
        m_isFadingIn = false;

        Engine::Input::UnregisterCursorUnlock("LoseScreen");
        Engine::Application::UnregisterPause("LoseScreen");
    }

    void LoseScreen::OnButtonClicked()
    {
        Engine::SoundManager::PlaySFX("ui_click");
        Hide();
        Engine::SceneManager::LoadWithLoadingScreen("MainMenu");
    }

    void LoseScreen::OnRestartButtonClicked()
    {
        Engine::SoundManager::PlaySFX("ui_click");
        Hide();
        Engine::SceneManager::LoadWithLoadingScreen("FPS");
    }
}