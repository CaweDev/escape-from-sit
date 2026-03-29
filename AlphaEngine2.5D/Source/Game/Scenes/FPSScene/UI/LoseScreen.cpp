/*!
 * \file AlphaEngine2.5D\Source\Game\Scenes\FPSScene\UI\LoseScreen.cpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief [AUTO] Added Doxygen file header
 */

#include "LoseScreen.hpp"
#include "Engine/Core/ConfigManager.hpp"

namespace Game
{
    namespace
    {
        struct LoseScreenConfig : public Engine::IConfig
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

            float restartButtonPosY = 125.0f;
            float mainMenuButtonPosY = 55.0f;
            float buttonWidth = 240.0f;
            float buttonHeight = 56.0f;
            float buttonPixelsPerUnitMultiplier = 1.0f;

            float fadeDuration = 0.5f;
            float uiClickSfxVolume = 1.0f;
            float loseSfxVolume = 1.0f;

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

                restartButtonPosY = j.get("restartButtonPosY", restartButtonPosY).asFloat();
                mainMenuButtonPosY = j.get("mainMenuButtonPosY", mainMenuButtonPosY).asFloat();
                buttonWidth = j.get("buttonWidth", buttonWidth).asFloat();
                buttonHeight = j.get("buttonHeight", buttonHeight).asFloat();
                buttonPixelsPerUnitMultiplier = j.get("buttonPixelsPerUnitMultiplier", buttonPixelsPerUnitMultiplier).asFloat();

                fadeDuration = j.get("fadeDuration", fadeDuration).asFloat();
                uiClickSfxVolume = j.get("uiClickSfxVolume", uiClickSfxVolume).asFloat();
                loseSfxVolume = j.get("loseSfxVolume", loseSfxVolume).asFloat();

                panelTexturePath = j.get("panelTexturePath", panelTexturePath).asString();
                buttonTexturePath = j.get("buttonTexturePath", buttonTexturePath).asString();
            }
        };

        LoseScreenConfig g_loseScreenConfig;
        bool g_loseScreenConfigLoaded = false;

        const LoseScreenConfig& GetLoseScreenConfig()
        {
            if (!g_loseScreenConfigLoaded)
            {
                Engine::ConfigManager::ReadConfig("Assets/Config/Game/lose_screen.json", g_loseScreenConfig);
                if (g_loseScreenConfig.fadeDuration < 0.0f) g_loseScreenConfig.fadeDuration = 0.0f;
                g_loseScreenConfigLoaded = true;
            }
            return g_loseScreenConfig;
        }
    }

    void LoseScreen::Awake()
    {
        BuildUI();
    }

    void LoseScreen::Update()
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

                if (m_pendingAction == PendingAction::LoadMainMenu)
                {
                    m_pendingAction = PendingAction::None;
                    Engine::SceneManager::LoadWithLoadingScreen("MainMenu");
                }
                else if (m_pendingAction == PendingAction::RestartFPSScene)
                {
                    m_pendingAction = PendingAction::None;
                    Engine::SceneManager::LoadWithLoadingScreen("FPS");
                }
            }
        }
    }

    void LoseScreen::ApplyFade(float t)
    {
        const auto& cfg = GetLoseScreenConfig();
        Engine::Color overlay = cfg.overlayColor;
        overlay.a *= t;
        if (m_overlay) m_overlay->SetBackgroundColor(overlay);

        Engine::Color panel = cfg.panelColor;
        panel.a *= t;
        if (m_panel) m_panel->SetBackgroundColor(panel);

        Engine::Color title = cfg.titleColor;
        title.a *= t;
        if (m_title) m_title->SetColor(title);

        Engine::Color btn = cfg.buttonColor;
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
        const auto& cfg = GetLoseScreenConfig();
        m_fadeDuration = cfg.fadeDuration;

        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        m_canvas->SetSortOrder(cfg.canvasSortOrder);
        m_canvas->SetReferenceResolution(cfg.referenceWidth, cfg.referenceHeight);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);

        m_overlay = m_canvas->CreateChild<UI::Panel>();
        m_overlay->name = "LoseOverlay";
        m_overlay->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_overlay->SetAnchoredPosition({ 0.0f, 0.0f });
        m_overlay->SetSizeDelta({ 0.0f, 0.0f });
        m_overlay->SetBackgroundColor(cfg.overlayColor);
        m_overlay->visible = false;

        m_panel = m_overlay->CreateChild<UI::Panel>();
        m_panel->name = "LosePanel";
        m_panel->rectTransform.SetCentered(cfg.panelWidth, cfg.panelHeight);
        m_panel->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.panelTexturePath.c_str()));
        m_panel->GetBackground()->SetImageType(UI::ImageType::Sliced);
        m_panel->GetBackground()->SetPixelsPerUnitMultiplier(cfg.panelPixelsPerUnitMultiplier);
        m_panel->SetBackgroundColor(cfg.panelColor);

        m_title = m_panel->CreateChild<UI::Text>();
        m_title->name = "LoseTitle";
        m_title->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        m_title->SetPivot({ 0.5f, 1.0f });
        m_title->SetAnchoredPosition({ 0.0f, cfg.titlePosY });
        m_title->SetSizeDelta({ 0.0f, cfg.titleHeight });
        m_title->SetText("YOU GOT CAUGHT. GAME OVER");
        m_title->SetFontSize(cfg.titleFontSize);
        m_title->SetColor(cfg.titleColor);
        m_title->SetAnchor(UI::TextAnchor::MiddleCenter);

        m_restartButton = m_panel->CreateChild<UI::Button>();
        m_restartButton->name = "LoseRestartButton";
        m_restartButton->SetAnchors({ 0.5f, 0.0f }, { 0.5f, 0.0f });
        m_restartButton->SetPivot({ 0.5f, 0.0f });
        m_restartButton->SetAnchoredPosition({ 0.0f, cfg.restartButtonPosY });
        m_restartButton->SetSizeDelta({ cfg.buttonWidth, cfg.buttonHeight });
        m_restartButton->SetText("Restart");
        m_restartButton->SetBackgroundColor(cfg.buttonColor);
        m_restartButton->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.buttonTexturePath.c_str()));
        m_restartButton->GetBackground()->SetImageType(UI::ImageType::Sliced);
        m_restartButton->GetBackground()->SetPixelsPerUnitMultiplier(cfg.buttonPixelsPerUnitMultiplier);
        m_restartButton->onClick = [this]() { OnRestartButtonClicked(); };

        m_button = m_panel->CreateChild<UI::Button>();
        m_button->name = "LoseButton";
        m_button->SetAnchors({ 0.5f, 0.0f }, { 0.5f, 0.0f });
        m_button->SetPivot({ 0.5f, 0.0f });
        m_button->SetAnchoredPosition({ 0.0f, cfg.mainMenuButtonPosY });
        m_button->SetSizeDelta({ cfg.buttonWidth, cfg.buttonHeight });
        m_button->SetText("Main Menu");
        m_button->SetBackgroundColor(cfg.buttonColor);
        m_button->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.buttonTexturePath.c_str()));
        m_button->GetBackground()->SetImageType(UI::ImageType::Sliced);
        m_button->GetBackground()->SetPixelsPerUnitMultiplier(cfg.buttonPixelsPerUnitMultiplier);
        m_button->onClick = [this]() { OnButtonClicked(); };

        ApplyFade(1.0f);
    }

    void LoseScreen::Show()
    {
        const auto& cfg = GetLoseScreenConfig();
        if (m_visible || !m_overlay) return;

        m_pendingAction = PendingAction::None;
        m_visible = true;
        m_overlay->visible = true;

        if (m_button) m_button->enabled = true;
        if (m_restartButton) m_restartButton->enabled = true;

        m_fadeTimer = 0.0f;
        m_isFadingOut = false;
        m_isFadingIn = true;
        ApplyFade(0.0f);

        Engine::Application::RegisterPause("LoseScreen");
        Engine::Input::RegisterCursorUnlock("LoseScreen");
        Engine::SoundManager::PlaySFX("lose", cfg.loseSfxVolume);
    }

    void LoseScreen::Hide()
    {
        if (!m_visible || !m_overlay || m_isFadingOut) return;

        m_isFadingIn = false;
        m_isFadingOut = true;
        m_fadeTimer = 0.0f;

        if (m_button) m_button->enabled = false;
        if (m_restartButton) m_restartButton->enabled = false;
    }

    void LoseScreen::CompleteHide()
    {
        if (!m_visible || !m_overlay) return;

        m_visible = false;
        m_overlay->visible = false;

        Engine::Input::UnregisterCursorUnlock("LoseScreen");
        Engine::Application::UnregisterPause("LoseScreen");
    }

    void LoseScreen::OnButtonClicked()
    {
        Engine::SoundManager::PlaySFX("ui_click", GetLoseScreenConfig().uiClickSfxVolume);
        m_pendingAction = PendingAction::LoadMainMenu;
        Hide();
    }

    void LoseScreen::OnRestartButtonClicked()
    {
        Engine::SoundManager::PlaySFX("ui_click", GetLoseScreenConfig().uiClickSfxVolume);
        m_pendingAction = PendingAction::RestartFPSScene;
        Hide();
    }
}