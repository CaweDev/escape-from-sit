#include "WinScreen.hpp"
#include "Game/Player/Player.hpp"
#include <cstdio>

namespace Game
{
    namespace
    {
        const Engine::Color kOverlayColor{ 0.0f, 0.0f, 0.0f, 0.75f };
        const Engine::Color kPanelColor{ 0.0f, 0.0f, 0.0f, 1.0f };
        const Engine::Color kTitleColor = Engine::Color::white();
        const Engine::Color kButtonColor{ 0.25f, 0.5f, 0.25f, 1.0f };

        void FormatTimeString(float timeSec, char* outBuffer, size_t outSize, const char* label)
        {
            if (!outBuffer || outSize == 0) return;

            if (timeSec < 0.0f || timeSec >= 900000.0f)
            {
                std::snprintf(outBuffer, outSize, "%s --:--", label);
                return;
            }

            int totalSeconds = static_cast<int>(timeSec);
            int minutes = totalSeconds / 60;
            int seconds = totalSeconds % 60;
            std::snprintf(outBuffer, outSize, "%s %02d:%02d", label, minutes, seconds);
        }
    }

    void WinScreen::Awake()
    {
        BuildUI();
    }

    void WinScreen::Update()
    {
        if (!m_visible) return;

        if (m_isFadingIn)
        {
            m_fadeTimer += Engine::Time::unscaledDeltaTime;
            float t = (m_fadeDuration > 0.0f) ? (m_fadeTimer / m_fadeDuration) : 1.0f;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            ApplyFade(t);

            if (t >= 1.0f)
            {
                m_isFadingIn = false;
            }
            return;
        }

        if (m_isFadingOut)
        {
            m_fadeTimer += Engine::Time::unscaledDeltaTime;
            float t = (m_fadeDuration > 0.0f) ? (1.0f - (m_fadeTimer / m_fadeDuration)) : 0.0f;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            ApplyFade(t);

            if (t <= 0.0f)
            {
                m_isFadingOut = false;
                CompleteHide();

                if (m_loadMainMenuAfterFade)
                {
                    m_loadMainMenuAfterFade = false;
                    Engine::SceneManager::LoadWithLoadingScreen("MainMenu");
                }
            }
        }
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
            m_timeSpentText = nullptr;
            m_highScoreText = nullptr;
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
        m_panel->SetBackgroundSprite(Engine::AssetManager::GetTexture("Assets/UI/UI Kit/Panel/panel-000.png"));
        m_panel->GetBackground()->SetImageType(UI::ImageType::Sliced);
        m_panel->GetBackground()->SetPixelsPerUnitMultiplier(4.0f);
        m_panel->SetBackgroundColor(kPanelColor);

        m_title = m_panel->CreateChild<UI::Text>();
        m_title->name = "WinTitle";
        m_title->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        m_title->SetPivot({ 0.5f, 1.0f });
        m_title->SetAnchoredPosition({ 0.0f, -55.0f });
        m_title->SetSizeDelta({ 0.0f, 64.0f });
        m_title->SetText("YOU ESCAPED. WIN!");
        m_title->SetFontSize(3.0f);
        m_title->SetColor(kTitleColor);
        m_title->SetAnchor(UI::TextAnchor::MiddleCenter);

        m_timeSpentText = m_panel->CreateChild<UI::Text>();
        m_timeSpentText->name = "WinTimeSpent";
        m_timeSpentText->SetAnchors({ 0.0f, 0.5f }, { 1.0f, 0.5f });
        m_timeSpentText->SetPivot({ 0.5f, 0.5f });
        m_timeSpentText->SetAnchoredPosition({ 0.0f, 20.0f });
        m_timeSpentText->SetSizeDelta({ 0.0f, 42.0f });
        m_timeSpentText->SetText("Time: 00:00");
        m_timeSpentText->SetFontSize(2.3f);
        m_timeSpentText->SetColor(kTitleColor);
        m_timeSpentText->SetAnchor(UI::TextAnchor::MiddleCenter);

        m_highScoreText = m_panel->CreateChild<UI::Text>();
        m_highScoreText->name = "WinHighScore";
        m_highScoreText->SetAnchors({ 0.0f, 0.5f }, { 1.0f, 0.5f });
        m_highScoreText->SetPivot({ 0.5f, 0.5f });
        m_highScoreText->SetAnchoredPosition({ 0.0f, -20.0f });
        m_highScoreText->SetSizeDelta({ 0.0f, 42.0f });
        m_highScoreText->SetText("Best: 00:00");
        m_highScoreText->SetFontSize(2.1f);
        m_highScoreText->SetColor(kTitleColor);
        m_highScoreText->SetAnchor(UI::TextAnchor::MiddleCenter);

        m_button = m_panel->CreateChild<UI::Button>();
        m_button->name = "WinButton";
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

    void WinScreen::ApplyFade(float t)
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
        if (m_timeSpentText) m_timeSpentText->SetColor(title);
        if (m_highScoreText) m_highScoreText->SetColor(title);

        Engine::Color btn = kButtonColor;
        btn.a *= t;
        if (m_button) m_button->SetBackgroundColor(btn);

        if (m_button && m_button->GetLabel())
        {
            Engine::Color c = Engine::Color::white();
            c.a = t;
            m_button->GetLabel()->SetColor(c);
        }
    }

    void WinScreen::Show()
    {
        if (m_visible || !m_overlay) return;

        auto* player = gameObject ? gameObject->GetComponent<Player>() : nullptr;
        if (player)
        {
            char currentTimeBuffer[64]{};
            char highScoreBuffer[64]{};

            FormatTimeString(player->GetGameTime(), currentTimeBuffer, sizeof(currentTimeBuffer), "Time:");
            FormatTimeString(player->GetHighScore(), highScoreBuffer, sizeof(highScoreBuffer), "Best:");

            if (m_timeSpentText) m_timeSpentText->SetText(currentTimeBuffer);
            if (m_highScoreText) m_highScoreText->SetText(highScoreBuffer);
        }

        m_loadMainMenuAfterFade = false;
        m_visible = true;
        m_overlay->visible = true;

        if (m_button) m_button->enabled = true;

        m_fadeTimer = 0.0f;
        m_isFadingOut = false;
        m_isFadingIn = true;
        ApplyFade(0.0f);

        Engine::Application::RegisterPause("WinScreen");
        Engine::Input::RegisterCursorUnlock("WinScreen");
        Engine::SoundManager::PlaySFX("win");
    }

    void WinScreen::Hide()
    {
        if (!m_visible || !m_overlay || m_isFadingOut) return;

        m_isFadingIn = false;
        m_isFadingOut = true;
        m_fadeTimer = 0.0f;

        if (m_button) m_button->enabled = false;
    }

    void WinScreen::CompleteHide()
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
        m_loadMainMenuAfterFade = true;
        Hide();
    }
}