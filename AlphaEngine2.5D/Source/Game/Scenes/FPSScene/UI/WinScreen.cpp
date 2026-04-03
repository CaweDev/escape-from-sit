/*!
 * \file AlphaEngine2.5D\Source\Game\Scenes\FPSScene\UI\WinScreen.cpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief [AUTO] Added Doxygen file header
 */

#include "WinScreen.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include "Game/Player/Player.hpp"
#include <cstdio>

namespace Game
{
    namespace
    {
        struct WinScreenConfig : public Engine::IConfig
        {
            Engine::Color overlayColor = Engine::Color(0.0f, 0.0f, 0.0f, 0.75f);
            Engine::Color panelColor = Engine::Color(0.0f, 0.0f, 0.0f, 1.0f);
            Engine::Color titleColor = Engine::Color::white();
            Engine::Color buttonColor = Engine::Color(0.25f, 0.5f, 0.25f, 1.0f);

            int canvasSortOrder = 1000;
            float referenceWidth = 1280.0f;
            float referenceHeight = 720.0f;

            float panelWidth = 1000.0f;
            float panelHeight = 560.0f;
            float panelPixelsPerUnitMultiplier = 4.0f;

            float titlePosY = -55.0f;
            float titleHeight = 64.0f;
            float titleFontSize = 3.0f;

            float timePosY = 20.0f;
            float highScorePosY = -20.0f;
            float statHeight = 42.0f;
            float timeFontSize = 2.3f;
            float highScoreFontSize = 2.1f;

            float buttonPosY = 55.0f;
            float buttonWidth = 240.0f;
            float buttonHeight = 56.0f;
            float buttonPixelsPerUnitMultiplier = 1.0f;

            float fadeDuration = 0.5f;
            float winSfxVolume = 1.0f;
            float uiClickSfxVolume = 1.0f;

            std::string panelTexturePath = "Assets/UI/UI Kit/Panel/panel-000.png";
            std::string buttonTexturePath = "Assets/UI/UI Kit/Panel/panel-027.png";

            static Engine::Color ParseColor(const Json::Value& j, const char* key, const Engine::Color& fallback)
            {
                if (!j.isMember(key) || !j[key].isObject()) return fallback;
                const auto& c = j[key];
                return Engine::Color(
                    c.get("r", fallback.r).asFloat(),
                    c.get("g", fallback.g).asFloat(),
                    c.get("b", fallback.b).asFloat(),
                    c.get("a", fallback.a).asFloat());
            }

            void FromJson(const Json::Value& j) override
            {
                overlayColor = ParseColor(j, "overlayColor", overlayColor);
                panelColor = ParseColor(j, "panelColor", panelColor);
                titleColor = ParseColor(j, "titleColor", titleColor);
                buttonColor = ParseColor(j, "buttonColor", buttonColor);

                canvasSortOrder = j.get("canvasSortOrder", canvasSortOrder).asInt();
                referenceWidth = j.get("referenceWidth", referenceWidth).asFloat();
                referenceHeight = j.get("referenceHeight", referenceHeight).asFloat();

                panelWidth = j.get("panelWidth", panelWidth).asFloat();
                panelHeight = j.get("panelHeight", panelHeight).asFloat();
                panelPixelsPerUnitMultiplier = j.get("panelPixelsPerUnitMultiplier", panelPixelsPerUnitMultiplier).asFloat();

                titlePosY = j.get("titlePosY", titlePosY).asFloat();
                titleHeight = j.get("titleHeight", titleHeight).asFloat();
                titleFontSize = j.get("titleFontSize", titleFontSize).asFloat();

                timePosY = j.get("timePosY", timePosY).asFloat();
                highScorePosY = j.get("highScorePosY", highScorePosY).asFloat();
                statHeight = j.get("statHeight", statHeight).asFloat();
                timeFontSize = j.get("timeFontSize", timeFontSize).asFloat();
                highScoreFontSize = j.get("highScoreFontSize", highScoreFontSize).asFloat();

                buttonPosY = j.get("buttonPosY", buttonPosY).asFloat();
                buttonWidth = j.get("buttonWidth", buttonWidth).asFloat();
                buttonHeight = j.get("buttonHeight", buttonHeight).asFloat();
                buttonPixelsPerUnitMultiplier = j.get("buttonPixelsPerUnitMultiplier", buttonPixelsPerUnitMultiplier).asFloat();

                fadeDuration = j.get("fadeDuration", fadeDuration).asFloat();
                winSfxVolume = j.get("winSfxVolume", winSfxVolume).asFloat();
                uiClickSfxVolume = j.get("uiClickSfxVolume", uiClickSfxVolume).asFloat();

                panelTexturePath = j.get("panelTexturePath", panelTexturePath).asString();
                buttonTexturePath = j.get("buttonTexturePath", buttonTexturePath).asString();
            }
        };

        WinScreenConfig g_winScreenConfig;
        bool g_winScreenConfigLoaded = false;

        const WinScreenConfig& GetWinScreenConfig()
        {
            if (!g_winScreenConfigLoaded)
            {
                Engine::ConfigManager::ReadConfig("Assets/Config/Game/win_screen.json", g_winScreenConfig);
                if (g_winScreenConfig.fadeDuration < 0.0f) g_winScreenConfig.fadeDuration = 0.0f;
                g_winScreenConfigLoaded = true;
            }
            return g_winScreenConfig;
        }

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
        const auto& cfg = GetWinScreenConfig();
        m_fadeDuration = cfg.fadeDuration;

        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        m_canvas->SetSortOrder(cfg.canvasSortOrder);
        m_canvas->SetReferenceResolution(cfg.referenceWidth, cfg.referenceHeight);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);

        m_overlay = m_canvas->CreateChild<UI::Panel>();
        m_overlay->name = "WinOverlay";
        m_overlay->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_overlay->SetAnchoredPosition({ 0.0f, 0.0f });
        m_overlay->SetSizeDelta({ 0.0f, 0.0f });
        m_overlay->SetBackgroundColor(cfg.overlayColor);
        m_overlay->visible = false;

        m_panel = m_overlay->CreateChild<UI::Panel>();
        m_panel->name = "WinPanel";
        m_panel->rectTransform.SetCentered(cfg.panelWidth, cfg.panelHeight);
        m_panel->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.panelTexturePath.c_str()));
        m_panel->GetBackground()->SetImageType(UI::ImageType::Sliced);
        m_panel->GetBackground()->SetPixelsPerUnitMultiplier(cfg.panelPixelsPerUnitMultiplier);
        m_panel->SetBackgroundColor(cfg.panelColor);

        m_title = m_panel->CreateChild<UI::Text>();
        m_title->name = "WinTitle";
        m_title->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        m_title->SetPivot({ 0.5f, 1.0f });
        m_title->SetAnchoredPosition({ 0.0f, cfg.titlePosY });
        m_title->SetSizeDelta({ 0.0f, cfg.titleHeight });
        m_title->SetText("YOU ESCAPED. WIN!");
        m_title->SetFontSize(cfg.titleFontSize);
        m_title->SetColor(cfg.titleColor);
        m_title->SetAnchor(UI::TextAnchor::MiddleCenter);

		// Time spent and high score texts
        m_timeSpentText = m_panel->CreateChild<UI::Text>();
        m_timeSpentText->name = "WinTimeSpent";
        m_timeSpentText->SetAnchors({ 0.0f, 0.5f }, { 1.0f, 0.5f });
        m_timeSpentText->SetPivot({ 0.5f, 0.5f });
        m_timeSpentText->SetAnchoredPosition({ 0.0f, cfg.timePosY });
        m_timeSpentText->SetSizeDelta({ 0.0f, cfg.statHeight });
        m_timeSpentText->SetText("Time: 00:00");
        m_timeSpentText->SetFontSize(cfg.timeFontSize);
        m_timeSpentText->SetColor(cfg.titleColor);
        m_timeSpentText->SetAnchor(UI::TextAnchor::MiddleCenter);

        m_highScoreText = m_panel->CreateChild<UI::Text>();
        m_highScoreText->name = "WinHighScore";
        m_highScoreText->SetAnchors({ 0.0f, 0.5f }, { 1.0f, 0.5f });
        m_highScoreText->SetPivot({ 0.5f, 0.5f });
        m_highScoreText->SetAnchoredPosition({ 0.0f, cfg.highScorePosY });
        m_highScoreText->SetSizeDelta({ 0.0f, cfg.statHeight });
        m_highScoreText->SetText("Best: 00:00");
        m_highScoreText->SetFontSize(cfg.highScoreFontSize);
        m_highScoreText->SetColor(cfg.titleColor);
        m_highScoreText->SetAnchor(UI::TextAnchor::MiddleCenter);

        m_button = m_panel->CreateChild<UI::Button>();
        m_button->name = "WinButton";
        m_button->SetAnchors({ 0.5f, 0.0f }, { 0.5f, 0.0f });
        m_button->SetPivot({ 0.5f, 0.0f });
        m_button->SetAnchoredPosition({ 0.0f, cfg.buttonPosY });
        m_button->SetSizeDelta({ cfg.buttonWidth, cfg.buttonHeight });
        m_button->SetText("Main Menu");
        m_button->SetBackgroundColor(cfg.buttonColor);
        m_button->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.buttonTexturePath.c_str()));
        m_button->GetBackground()->SetImageType(UI::ImageType::Sliced);
        m_button->GetBackground()->SetPixelsPerUnitMultiplier(cfg.buttonPixelsPerUnitMultiplier);
        m_button->onClick = [this]() { OnButtonClicked(); };

        ApplyFade(1.0f);
    }

    void WinScreen::ApplyFade(float t)
    {
        const auto& cfg = GetWinScreenConfig();
        Engine::Color overlay = cfg.overlayColor;
        overlay.a *= t;
        if (m_overlay) m_overlay->SetBackgroundColor(overlay);

        Engine::Color panel = cfg.panelColor;
        panel.a *= t;
        if (m_panel) m_panel->SetBackgroundColor(panel);

        Engine::Color title = cfg.titleColor;
        title.a *= t;
        if (m_title) m_title->SetColor(title);
        if (m_timeSpentText) m_timeSpentText->SetColor(title);
        if (m_highScoreText) m_highScoreText->SetColor(title);

        Engine::Color btn = cfg.buttonColor;
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
        const auto& cfg = GetWinScreenConfig();
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
        Engine::SoundManager::PlaySFX("win", cfg.winSfxVolume);
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
        Engine::SoundManager::PlaySFX("ui_click", GetWinScreenConfig().uiClickSfxVolume);
        m_loadMainMenuAfterFade = true;
        Hide();
    }
}