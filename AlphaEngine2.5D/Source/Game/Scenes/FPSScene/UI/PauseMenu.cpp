/**
 * @file PauseMenu.cpp
 * @brief Implementation of the PauseMenu component
 */

#include "PauseMenu.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <iostream>

namespace Game
{
    namespace
    {
        bool TryReadVec2(const Json::Value& node, const char* key, AEVec2& out)
        {
            if (!node.isMember(key) || !node[key].isObject()) return false;
            const auto& v = node[key];
            out.x = v.get("x", out.x).asFloat();
            out.y = v.get("y", out.y).asFloat();
            return true;
        }

        void ApplyLayoutNode(UI::UIElement* e, const Json::Value& layout)
        {
            if (!e || e->name.empty() || !layout.isObject()) return;
            const auto& n = layout[e->name];
            if (!n.isObject()) return;

            AEVec2 v{};
            v = e->GetAnchoredPosition(); if (TryReadVec2(n, "anchoredPosition", v)) e->SetAnchoredPosition(v);
            v = e->GetSizeDelta();        if (TryReadVec2(n, "sizeDelta", v))        e->SetSizeDelta(v);
            v = e->GetPivot();            if (TryReadVec2(n, "pivot", v))            e->SetPivot(v);
            AEVec2 amin = e->GetAnchorMin(), amax = e->GetAnchorMax();
            bool hasMin = TryReadVec2(n, "anchorMin", amin);
            bool hasMax = TryReadVec2(n, "anchorMax", amax);
            if (hasMin || hasMax) e->SetAnchors(amin, amax);
            if (n.isMember("renderOrder")) e->SetRenderOrder(n["renderOrder"].asInt());
        }

        void ApplyLayoutRecursive(UI::UIElement* e, const Json::Value& layout)
        {
            if (!e) return;
            ApplyLayoutNode(e, layout);
            for (auto* c : e->GetChildren()) ApplyLayoutRecursive(c, layout);
        }

        void ApplyLayoutToCanvas(UI::Canvas* canvas, const Json::Value& layout)
        {
            if (!canvas || !layout.isObject()) return;
            for (auto* root : canvas->GetRootElements()) ApplyLayoutRecursive(root, layout);
        }

        struct PauseMenuConfig : public Engine::IConfig
        {
            Json::Value layout;
            Engine::Color overlayColor = Engine::Color(0.0f, 0.0f, 0.0f, 0.75f);
            Engine::Color menuBgColor = Engine::Color(0.3f, 0.12f, 0.12f, 0.92f);
            Engine::Color titleColor = Engine::Color(0.8f, 0.15f, 0.10f, 1.0f);
            Engine::Color dividerColor = Engine::Color(0.6f, 0.10f, 0.08f, 0.7f);

            Engine::Color btnNormal = Engine::Color(0.12f, 0.05f, 0.05f, 0.9f);
            Engine::Color btnHighlighted = Engine::Color(0.22f, 0.08f, 0.08f, 1.0f);
            Engine::Color btnPressed = Engine::Color(0.30f, 0.10f, 0.10f, 1.0f);
            Engine::Color btnDisabled = Engine::Color(0.08f, 0.04f, 0.04f, 0.5f);

            Engine::Color textNormal = Engine::Color(0.6f, 0.6f, 0.6f, 1.0f);
            Engine::Color textSelected = Engine::Color(1.0f, 1.0f, 1.0f, 1.0f);

            float menuWidth = 400.0f;
            float menuHeight = 400.0f;
            float btnWidth = 280.0f;
            float btnHeight = 52.0f;
            float btnSpacing = 16.0f;

            float fontSizeBase = 1.8f;
            float fontSizeSelectedScale = 1.15f;
            float fontLerpSpeed = 10.0f;

            float referenceWidth = 1280.0f;
            float referenceHeight = 720.0f;
            int canvasSortOrder = 500;
            int overlayRenderOrder = 0;
            int menuRenderOrder = 1;
            int titleRenderOrder = 2;
            int buttonRenderOrder = 3;
            float titleFontSize = 3.5f;
            float titleDividerThickness = 2.0f;
            float panelPixelsPerUnitMultiplier = 4.0f;
            float buttonPixelsPerUnitMultiplier = 1.0f;
            float uiHoverSfxVolume = 0.7f;
            float uiToggleSfxVolume = 0.8f;

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
                layout = j["layout"];
                overlayColor = ParseColor(j, "overlayColor", overlayColor);
                menuBgColor = ParseColor(j, "menuBgColor", menuBgColor);
                titleColor = ParseColor(j, "titleColor", titleColor);
                dividerColor = ParseColor(j, "dividerColor", dividerColor);

                btnNormal = ParseColor(j, "btnNormal", btnNormal);
                btnHighlighted = ParseColor(j, "btnHighlighted", btnHighlighted);
                btnPressed = ParseColor(j, "btnPressed", btnPressed);
                btnDisabled = ParseColor(j, "btnDisabled", btnDisabled);

                textNormal = ParseColor(j, "textNormal", textNormal);
                textSelected = ParseColor(j, "textSelected", textSelected);

                menuWidth = j.get("menuWidth", menuWidth).asFloat();
                menuHeight = j.get("menuHeight", menuHeight).asFloat();
                btnWidth = j.get("btnWidth", btnWidth).asFloat();
                btnHeight = j.get("btnHeight", btnHeight).asFloat();
                btnSpacing = j.get("btnSpacing", btnSpacing).asFloat();

                fontSizeBase = j.get("fontSizeBase", fontSizeBase).asFloat();
                fontSizeSelectedScale = j.get("fontSizeSelectedScale", fontSizeSelectedScale).asFloat();
                fontLerpSpeed = j.get("fontLerpSpeed", fontLerpSpeed).asFloat();

                referenceWidth = j.get("referenceWidth", referenceWidth).asFloat();
                referenceHeight = j.get("referenceHeight", referenceHeight).asFloat();
                canvasSortOrder = j.get("canvasSortOrder", canvasSortOrder).asInt();
                overlayRenderOrder = j.get("overlayRenderOrder", overlayRenderOrder).asInt();
                menuRenderOrder = j.get("menuRenderOrder", menuRenderOrder).asInt();
                titleRenderOrder = j.get("titleRenderOrder", titleRenderOrder).asInt();
                buttonRenderOrder = j.get("buttonRenderOrder", buttonRenderOrder).asInt();
                titleFontSize = j.get("titleFontSize", titleFontSize).asFloat();
                titleDividerThickness = j.get("titleDividerThickness", titleDividerThickness).asFloat();
                panelPixelsPerUnitMultiplier = j.get("panelPixelsPerUnitMultiplier", panelPixelsPerUnitMultiplier).asFloat();
                buttonPixelsPerUnitMultiplier = j.get("buttonPixelsPerUnitMultiplier", buttonPixelsPerUnitMultiplier).asFloat();
                uiHoverSfxVolume = j.get("uiHoverSfxVolume", uiHoverSfxVolume).asFloat();
                uiToggleSfxVolume = j.get("uiToggleSfxVolume", uiToggleSfxVolume).asFloat();

                panelTexturePath = j.get("panelTexturePath", panelTexturePath).asString();
                buttonTexturePath = j.get("buttonTexturePath", buttonTexturePath).asString();
            }
        };

        PauseMenuConfig g_pauseMenuConfig;
        bool g_pauseMenuConfigLoaded = false;

        const PauseMenuConfig& GetPauseMenuConfig()
        {
            if (!g_pauseMenuConfigLoaded)
            {
                Engine::ConfigManager::ReadConfig("Assets/Config/Game/pause_menu.json", g_pauseMenuConfig);
                g_pauseMenuConfigLoaded = true;
            }
            return g_pauseMenuConfig;
        }
    }

    // ────────────────────────────────────────────────────────────
    //  Lifecycle
    // ────────────────────────────────────────────────────────────

    PauseMenu::~PauseMenu()
    {
        // Canvas and children are cleaned up by UIManager
    }

    void PauseMenu::Awake()
    {
        BuildUI();
    }

    void PauseMenu::Update()
    {
        const auto& cfg = GetPauseMenuConfig();
        // Toggle on Escape (uses unscaledDeltaTime-independent key check)
        if (Engine::Input::GetKeyDown(Engine::KeyCode::Escape))
        {
            Toggle();
        }

        if (!m_isPaused) return;

        float dt = Engine::Time::unscaledDeltaTime;

        // Skip input processing while a confirmation dialog is open
        if (UI::DialogService::IsConfirmationVisible())
        {
            UpdateMenuAnimations(dt);
            return;
        }

        // Keyboard navigation
        int itemCount = static_cast<int>(m_items.size());
        if (itemCount > 0)
        {
            if (Engine::Input::GetKeyDown(Engine::KeyCode::W) ||
                Engine::Input::GetKeyDown(Engine::KeyCode::UpArrow))
            {
                m_selectedIndex = (m_selectedIndex - 1 + itemCount) % itemCount;
                Engine::SoundManager::PlaySFX("ui_hover", cfg.uiHoverSfxVolume);
                UpdateMenuVisuals();
            }

            if (Engine::Input::GetKeyDown(Engine::KeyCode::S) ||
                Engine::Input::GetKeyDown(Engine::KeyCode::DownArrow))
            {
                m_selectedIndex = (m_selectedIndex + 1) % itemCount;
                Engine::SoundManager::PlaySFX("ui_hover", cfg.uiHoverSfxVolume);
                UpdateMenuVisuals();
            }

            if (Engine::Input::GetKeyDown(Engine::KeyCode::Return) ||
                Engine::Input::GetKeyDown(Engine::KeyCode::Space))
            {
                SelectMenuItem(m_selectedIndex);
            }
        }

        // Mouse hover detection
        {
            Engine::Vector2 mousePos = Engine::Input::GetMousePosition();
            auto canvasPos = m_canvas->ScreenToCanvasPosition(
                static_cast<s32>(mousePos.x), static_cast<s32>(mousePos.y));
            f32 worldX = canvasPos.x, worldY = canvasPos.y;

            int newHovered = -1;
            for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
            {
                UI::Rect btnRect = m_items[i].button->GetWorldRect();
                if (worldX >= btnRect.x && worldX <= btnRect.x + btnRect.width &&
                    worldY >= btnRect.y && worldY <= btnRect.y + btnRect.height)
                {
                    newHovered = i;
                    break;
                }
            }

            if (newHovered != m_hoveredIndex)
            {
                m_hoveredIndex = newHovered;
                if (m_hoveredIndex >= 0)
                {
                    m_selectedIndex = m_hoveredIndex;
                    Engine::SoundManager::PlaySFX("ui_hover", cfg.uiHoverSfxVolume);
                }
                UpdateMenuVisuals();
            }

            // Mouse press — record which button was pressed
            if (Engine::Input::GetMouseButtonDown(0) && m_hoveredIndex >= 0)
            {
                m_pressedIndex = m_hoveredIndex;
            }

            // Mouse release — activate only if still hovering the same button
            if (Engine::Input::GetMouseButtonUp(0) && m_pressedIndex >= 0)
            {
                if (m_pressedIndex == m_hoveredIndex)
                {
                    SelectMenuItem(m_pressedIndex);
                }
                m_pressedIndex = -1;
            }
        }

        // Smooth font-size animation
        UpdateMenuAnimations(dt);
    }

    void PauseMenu::OnDestroy()
    {
        // Ensure pause is always cleaned up when the scene unloads
        if (m_isPaused)
        {
            Engine::Input::UnregisterCursorUnlock("PauseMenu");
            Engine::Application::UnregisterPause("PauseMenu");
            m_isPaused = false;
        }

        if (m_canvas)
        {
            UI::UIManager::GetInstance()->DestroyCanvas(m_canvas);
            m_canvas      = nullptr;
            m_overlay     = nullptr;
            m_menuPanel   = nullptr;
            m_title       = nullptr;
            m_divider     = nullptr;
            m_resumeBtn   = nullptr;
            m_restartBtn  = nullptr;
            m_mainMenuBtn = nullptr;
        }
        m_items.clear();
    }

    // ────────────────────────────────────────────────────────────
    //  Show / Hide / Toggle
    // ────────────────────────────────────────────────────────────

    void PauseMenu::Show()
    {
        const auto& cfg = GetPauseMenuConfig();
        if (m_isPaused) return;

        m_isPaused = true;
        m_selectedIndex = 0;

        if (m_overlay)  m_overlay->visible  = true;

        // Pause game
        Engine::Application::RegisterPause("PauseMenu");

        // Unlock cursor so the player can click buttons
        Engine::Input::RegisterCursorUnlock("PauseMenu");

        Engine::SoundManager::PlaySFX("ui_toggle", cfg.uiToggleSfxVolume);
        UpdateMenuVisuals();
    }

    void PauseMenu::Hide()
    {
        const auto& cfg = GetPauseMenuConfig();
        if (!m_isPaused) return;

        m_isPaused = false;

        if (m_overlay) m_overlay->visible = false;

        // Unpause game
        Engine::Application::UnregisterPause("PauseMenu");

        // Re-lock cursor for FPS controls
        Engine::Input::UnregisterCursorUnlock("PauseMenu");
        Engine::SoundManager::PlaySFX("ui_toggle", cfg.uiToggleSfxVolume);
    }

    void PauseMenu::Toggle()
    {
        if (m_isPaused) Hide();
        else            Show();
    }

    // ────────────────────────────────────────────────────────────
    //  UI Construction
    // ────────────────────────────────────────────────────────────

    void PauseMenu::BuildUI()
    {
        const auto& cfg = GetPauseMenuConfig();
        // --- Canvas ---
        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        m_canvas->SetReferenceResolution(cfg.referenceWidth, cfg.referenceHeight);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);
        m_canvas->SetSortOrder(cfg.canvasSortOrder);  // Above HUD (100), below end-panel (1000)

        // --- Full-screen overlay ---
        m_overlay = m_canvas->CreateChild<UI::Panel>();
        m_overlay->name = "PauseOverlay";
        m_overlay->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_overlay->SetAnchoredPosition({ 0.0f, 0.0f });
        m_overlay->SetSizeDelta({ 0.0f, 0.0f });
        m_overlay->SetBackgroundColor(cfg.overlayColor);
        m_overlay->SetRenderOrder(cfg.overlayRenderOrder);
        m_overlay->visible = false;  // Start hidden

        // --- Centered menu panel ---
        m_menuPanel = m_overlay->CreateChild<UI::Panel>();
        m_menuPanel->name = "PauseMenuPanel";
        m_menuPanel->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
        m_menuPanel->SetPivot({ 0.5f, 0.5f });
        m_menuPanel->SetAnchoredPosition({ 0.0f, 0.0f });
        m_menuPanel->SetSizeDelta({ cfg.menuWidth, cfg.menuHeight });
        m_menuPanel->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.panelTexturePath.c_str()));
        m_menuPanel->GetBackground()->SetImageType(UI::ImageType::Sliced);
        m_menuPanel->GetBackground()->SetPixelsPerUnitMultiplier(cfg.panelPixelsPerUnitMultiplier);
        m_menuPanel->SetBackgroundColor(cfg.menuBgColor);
        m_menuPanel->SetRenderOrder(cfg.menuRenderOrder);

        // --- Title text: "PAUSED" ---
        m_title = m_menuPanel->CreateChild<UI::Text>();
        m_title->name = "PauseTitle";
        m_title->SetAnchors({ 0.0f, 0.7f }, { 1.0f, 0.95f });
        m_title->SetAnchoredPosition({ 0.0f, 0.0f });
        m_title->SetSizeDelta({ 0.0f, 0.0f });
        m_title->SetText("PAUSED");
        m_title->SetFontSize(cfg.titleFontSize);
        m_title->SetColor(cfg.titleColor);
        m_title->SetAnchor(UI::TextAnchor::MiddleCenter);
        m_title->SetRenderOrder(cfg.titleRenderOrder);

        // --- Divider line ---
        m_divider = m_menuPanel->CreateChild<UI::Image>();
        m_divider->name = "PauseDivider";
        m_divider->SetAnchors({ 0.15f, 0.74f }, { 0.85f, 0.74f });
        m_divider->SetPivot({ 0.5f, 0.5f });
        m_divider->SetAnchoredPosition({ 0.0f, 0.0f });
        m_divider->SetSizeDelta({ 0.0f, cfg.titleDividerThickness });
        m_divider->SetColor(cfg.dividerColor);
        m_divider->SetRenderOrder(cfg.titleRenderOrder);

        // --- Button style ---
        UI::ColorBlock btnColors;
        btnColors.normalColor      = cfg.btnNormal;
        btnColors.highlightedColor = cfg.btnHighlighted;
        btnColors.pressedColor     = cfg.btnPressed;
        btnColors.disabledColor    = cfg.btnDisabled;
        btnColors.fadeDuration     = 0.12f;

        // --- Button configs ---
        struct BtnConfig
        {
            const char* label;
            std::function<void()> callback;
        };

        BtnConfig configs[] = {
            { "Resume",    [this]() { OnResumeClicked();   } },
            { "Restart",   [this]() { OnRestartClicked();  } },
            { "Main Menu", [this]() { OnMainMenuClicked(); } },
        };

        constexpr int btnCount = 3;

        // Position buttons vertically from top of lower region
        // Available vertical space: anchors 0.10 .. 0.70 of the menu panel
        const float regionTop    = 0.66f;
        const float regionBottom = 0.10f;
        const float regionH     = regionTop - regionBottom;
        const float btnNormH    = cfg.btnHeight / cfg.menuHeight;          // button height in anchor space
        const float spacing     = (regionH - btnCount * btnNormH) / (btnCount + 1);

        for (int i = 0; i < btnCount; ++i)
        {
            float btnTop = regionTop - spacing * (i + 1) - btnNormH * i;
            float btnBot = btnTop - btnNormH;

            UI::Button* btn = m_menuPanel->CreateChild<UI::Button>();
            btn->name = configs[i].label;
            btn->SetAnchors({ 0.5f, btnBot }, { 0.5f, btnTop });
            btn->SetPivot({ 0.5f, 0.5f });
            btn->SetAnchoredPosition({ 0.0f, 0.0f });
            btn->SetSizeDelta({ cfg.btnWidth, 0.0f });
            btn->SetColors(btnColors);
            btn->SetBackgroundColor(cfg.btnNormal);
            btn->SetText(configs[i].label);
            btn->onClick = configs[i].callback;
            btn->SetRenderOrder(cfg.buttonRenderOrder);
            btn->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.buttonTexturePath.c_str()));
            btn->GetBackground()->SetImageType(UI::ImageType::Sliced);
            btn->GetBackground()->SetPixelsPerUnitMultiplier(cfg.buttonPixelsPerUnitMultiplier);

            // Style the label text
            if (auto* label = btn->GetLabel())
            {
                label->SetFontSize(cfg.fontSizeBase);
                label->SetColor(cfg.textNormal);
            }

            // Track for keyboard nav
            MenuItem item;
            item.button          = btn;
            item.label           = configs[i].label;
            item.baseFontSize    = cfg.fontSizeBase;
            item.currentFontSize = cfg.fontSizeBase;
            item.targetFontSize  = cfg.fontSizeBase;
            m_items.push_back(item);
        }

        // Keep pointers for direct access
        m_resumeBtn   = m_items[0].button;
        m_restartBtn  = m_items[1].button;
        m_mainMenuBtn = m_items[2].button;

        ApplyLayoutToCanvas(m_canvas, cfg.layout);
    }

    // ────────────────────────────────────────────────────────────
    //  Visual Updates
    // ────────────────────────────────────────────────────────────

    void PauseMenu::UpdateMenuVisuals()
    {
        const auto& cfg = GetPauseMenuConfig();
        for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
        {
            bool selected = (i == m_selectedIndex);
            auto* label = m_items[i].button->GetLabel();
            if (!label) continue;

            if (selected)
            {
                label->SetText("> " + m_items[i].label);
                label->SetColor(cfg.textSelected);
                m_items[i].targetFontSize = m_items[i].baseFontSize * cfg.fontSizeSelectedScale;
            }
            else
            {
                label->SetText(m_items[i].label);
                label->SetColor(cfg.textNormal);
                m_items[i].targetFontSize = m_items[i].baseFontSize;
            }
        }
    }

    void PauseMenu::UpdateMenuAnimations(float dt)
    {
        const auto& cfg = GetPauseMenuConfig();
        for (auto& item : m_items)
        {
            item.currentFontSize += (item.targetFontSize - item.currentFontSize) * dt * cfg.fontLerpSpeed;

            if (auto* label = item.button->GetLabel())
            {
                label->SetFontSize(item.currentFontSize);
            }
        }
    }

    void PauseMenu::SelectMenuItem(int index)
    {
        const auto& cfg = GetPauseMenuConfig();
        if (index < 0 || index >= static_cast<int>(m_items.size())) return;

        if (m_items[index].button && m_items[index].button->onClick)
        {
            Engine::SoundManager::PlaySFX("ui_click", cfg.uiToggleSfxVolume);
            m_items[index].button->onClick();
        }
    }

    // ────────────────────────────────────────────────────────────
    //  Button Callbacks
    // ────────────────────────────────────────────────────────────

    void PauseMenu::OnResumeClicked()
    {
        Hide();
    }

    void PauseMenu::OnRestartClicked()
    {
        UI::DialogService::ShowConfirmation(
            "Restart the level?",
            "Confirm", "Cancel",
            [this]()
            {
                //Engine::Input::UnregisterCursorUnlock("PauseMenu"); // moved to OnDestroy()
                Engine::SceneManager::LoadWithLoadingScreen("FPS");
            }
        );
    }

    void PauseMenu::OnMainMenuClicked()
    {
        UI::DialogService::ShowConfirmation(
            "Return to Main Menu?",
            "Confirm", "Cancel",
            [this]()
            {
                //Engine::Input::UnregisterCursorUnlock("PauseMenu"); // moved to OnDestroy()
                Engine::SceneManager::LoadWithLoadingScreen("MainMenu");
            }
        );
    }

} // namespace Game
