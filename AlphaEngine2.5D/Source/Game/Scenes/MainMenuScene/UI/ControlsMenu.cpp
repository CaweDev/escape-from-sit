#include "ControlsMenu.hpp"
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

        struct ControlsMenuConfig : public Engine::IConfig
        {
            Json::Value layout;
            Engine::Color overlayColor = Engine::Color(0.0f, 0.0f, 0.0f, 0.85f);
            Engine::Color btnNormal = Engine::Color(0.75f, 0.08f, 0.08f, 1.0f);
            Engine::Color btnHighlighted = Engine::Color(0.95f, 0.08f, 0.08f, 1.0f);
            Engine::Color btnPressed = Engine::Color(0.30f, 0.10f, 0.10f, 1.0f);
            Engine::Color textNormal = Engine::Color(0.9f, 0.9f, 0.9f, 1.0f);
            Engine::Color textSelected = Engine::Color(1.0f, 1.0f, 1.0f, 1.0f);
            Engine::Color panelTint = Engine::Color(1.0f, 1.0f, 1.0f, 1.0f);

            float referenceWidth = 1280.0f;
            float referenceHeight = 720.0f;
            int canvasSortOrder = 600;

            float panelWidth = 1120.0f;
            float panelHeight = 760.0f;
            float panelPixelsPerUnitMultiplier = 4.0f;

            float controlsPosY = -20.0f;
            float controlsWidth = 980.0f;
            float controlsHeight = 620.0f;

            float backButtonPosY = 20.0f;
            float backButtonWidth = 240.0f;
            float backButtonHeight = 50.0f;
            float backButtonPixelsPerUnitMultiplier = 1.0f;
            float backButtonFontSize = 2.0f;

            float uiToggleSfxVolume = 0.8f;
            float uiHoverSfxVolume = 0.7f;

            std::string panelTexturePath = "Assets/UI/UI Kit/Panel/panel-000.png";
            std::string controlsTexturePath = "Assets/UI/Controls.png";
            std::string buttonTexturePath = "Assets/UI/UI Kit/Panel/panel-027.png";

            static Engine::Color ParseColor(const Json::Value& j, const char* key, const Engine::Color& fallback)
            {
                if (!j.isMember(key) || !j[key].isObject()) return fallback;
                const Json::Value& c = j[key];
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
                btnNormal = ParseColor(j, "btnNormal", btnNormal);
                btnHighlighted = ParseColor(j, "btnHighlighted", btnHighlighted);
                btnPressed = ParseColor(j, "btnPressed", btnPressed);
                textNormal = ParseColor(j, "textNormal", textNormal);
                textSelected = ParseColor(j, "textSelected", textSelected);
                panelTint = ParseColor(j, "panelTint", panelTint);

                referenceWidth = j.get("referenceWidth", referenceWidth).asFloat();
                referenceHeight = j.get("referenceHeight", referenceHeight).asFloat();
                canvasSortOrder = j.get("canvasSortOrder", canvasSortOrder).asInt();

                panelWidth = j.get("panelWidth", panelWidth).asFloat();
                panelHeight = j.get("panelHeight", panelHeight).asFloat();
                panelPixelsPerUnitMultiplier = j.get("panelPixelsPerUnitMultiplier", panelPixelsPerUnitMultiplier).asFloat();

                controlsPosY = j.get("controlsPosY", controlsPosY).asFloat();
                controlsWidth = j.get("controlsWidth", controlsWidth).asFloat();
                controlsHeight = j.get("controlsHeight", controlsHeight).asFloat();

                backButtonPosY = j.get("backButtonPosY", backButtonPosY).asFloat();
                backButtonWidth = j.get("backButtonWidth", backButtonWidth).asFloat();
                backButtonHeight = j.get("backButtonHeight", backButtonHeight).asFloat();
                backButtonPixelsPerUnitMultiplier = j.get("backButtonPixelsPerUnitMultiplier", backButtonPixelsPerUnitMultiplier).asFloat();
                backButtonFontSize = j.get("backButtonFontSize", backButtonFontSize).asFloat();

                uiToggleSfxVolume = j.get("uiToggleSfxVolume", uiToggleSfxVolume).asFloat();
                uiHoverSfxVolume = j.get("uiHoverSfxVolume", uiHoverSfxVolume).asFloat();

                panelTexturePath = j.get("panelTexturePath", panelTexturePath).asString();
                controlsTexturePath = j.get("controlsTexturePath", controlsTexturePath).asString();
                buttonTexturePath = j.get("buttonTexturePath", buttonTexturePath).asString();
            }
        };

        ControlsMenuConfig g_controlsMenuConfig;
        bool g_controlsMenuConfigLoaded = false;

        const ControlsMenuConfig& GetControlsMenuConfig()
        {
            if (!g_controlsMenuConfigLoaded)
            {
                Engine::ConfigManager::ReadConfig("Assets/Config/Game/controls_menu.json", g_controlsMenuConfig);
                g_controlsMenuConfigLoaded = true;
            }
            return g_controlsMenuConfig;
        }

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
        const auto& cfg = GetControlsMenuConfig();

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
                Engine::SoundManager::PlaySFX("ui_hover", cfg.uiHoverSfxVolume);
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
        const auto& cfg = GetControlsMenuConfig();

        Engine::Application::RegisterPause(kPauseId);
        Engine::Input::RegisterCursorUnlock(kCursorUnlockId);
        if (m_overlay) m_overlay->visible = true;
        Engine::SoundManager::PlaySFX("ui_toggle", cfg.uiToggleSfxVolume);
    }

    void ControlsMenu::Hide()
    {
        if (!m_isVisible) return;
        m_isVisible = false;
        const auto& cfg = GetControlsMenuConfig();

        Engine::Application::UnregisterPause(kPauseId);
        Engine::Input::UnregisterCursorUnlock(kCursorUnlockId);
        if (m_overlay) m_overlay->visible = false;
        Engine::SoundManager::PlaySFX("ui_toggle", cfg.uiToggleSfxVolume);
    }

    void ControlsMenu::Toggle()
    {
        if (m_isVisible) Hide();
        else             Show();
    }

    void ControlsMenu::BuildUI()
    {
        const auto& cfg = GetControlsMenuConfig();
        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        m_canvas->SetReferenceResolution(cfg.referenceWidth, cfg.referenceHeight);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);
        m_canvas->SetSortOrder(cfg.canvasSortOrder); // Above Main Menu and Pause Menu

        // Full screen overlay
        m_overlay = m_canvas->CreateChild<UI::Panel>();
        m_overlay->name = "ControlsOverlay";
        m_overlay->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_overlay->SetAnchoredPosition({ 0.0f, 0.0f });
        m_overlay->SetSizeDelta({ 0.0f, 0.0f });
        m_overlay->SetBackgroundColor(cfg.overlayColor);
        m_overlay->visible = false;

        // Background Panel from UI Kit
        m_panelImage = m_overlay->CreateChild<UI::Image>();
        m_panelImage->name = "ControlsPanelBase";
        m_panelImage->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
        m_panelImage->SetPivot({ 0.5f, 0.5f });
        m_panelImage->SetAnchoredPosition({ 0.0f, 0.0f });
        m_panelImage->SetSizeDelta({ cfg.panelWidth, cfg.panelHeight });
        m_panelImage->SetTexture(Engine::AssetManager::GetTexture(cfg.panelTexturePath.c_str()));
		m_panelImage->SetImageType(UI::ImageType::Sliced);
		m_panelImage->SetPixelsPerUnitMultiplier(cfg.panelPixelsPerUnitMultiplier); // Make the 9-slice borders thicker
		m_panelImage->SetColor(cfg.panelTint); // Slightly transparent to show overlay tint

        // Controls Image
        m_controlsImage = m_panelImage->CreateChild<UI::Image>();
        m_controlsImage->name = "ControlsContent";
        m_controlsImage->SetAnchors({ 0.5f, 0.55f }, { 0.5f, 0.55f });
        m_controlsImage->SetPivot({ 0.5f, 0.5f });
        m_controlsImage->SetAnchoredPosition({ 0.0f, cfg.controlsPosY });
        m_controlsImage->SetSizeDelta({ cfg.controlsWidth, cfg.controlsHeight });
        m_controlsImage->SetTexture(Engine::AssetManager::GetTexture(cfg.controlsTexturePath.c_str()));

        // Back Button
        m_backBtn = m_panelImage->CreateChild<UI::Button>();
        m_backBtn->name = "Close";
        m_backBtn->SetAnchors({ 0.5f, 0.0f }, { 0.5f, 0.0f });
        m_backBtn->SetPivot({ 0.5f, 0.5f });
        m_backBtn->SetAnchoredPosition({ 0.0f, cfg.backButtonPosY });
        m_backBtn->SetSizeDelta({ cfg.backButtonWidth, cfg.backButtonHeight });
        m_backBtn->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.buttonTexturePath.c_str()));
        m_backBtn->GetBackground()->SetImageType(UI::ImageType::Sliced);
        m_backBtn->GetBackground()->SetPixelsPerUnitMultiplier(cfg.backButtonPixelsPerUnitMultiplier);
        
        UI::ColorBlock btnColors;
        btnColors.normalColor      = cfg.btnNormal;
        btnColors.highlightedColor = cfg.btnHighlighted;
        btnColors.pressedColor     = cfg.btnPressed;
        
        m_backBtn->SetColors(btnColors);
        m_backBtn->SetBackgroundColor(cfg.btnNormal);
        m_backBtn->SetText("Back");
        m_backBtn->onClick = [this]() { OnBackClicked(); };
        
        if (auto* label = m_backBtn->GetLabel())
        {
            label->SetFontSize(cfg.backButtonFontSize);
            label->SetColor(cfg.textNormal);
        }

        ApplyLayoutToCanvas(m_canvas, cfg.layout);
    }

    void ControlsMenu::UpdateButtonVisuals()
    {
        if (!m_backBtn) return;
        const auto& cfg = GetControlsMenuConfig();
        
        if (m_isPressed)
        {
            m_backBtn->SetBackgroundColor(cfg.btnPressed);
            if (auto* lbl = m_backBtn->GetLabel()) lbl->SetColor(cfg.textSelected);
        }
        else if (m_isHovered)
        {
            m_backBtn->SetBackgroundColor(cfg.btnHighlighted);
            if (auto* lbl = m_backBtn->GetLabel()) lbl->SetColor(cfg.textSelected);
        }
        else
        {
            m_backBtn->SetBackgroundColor(cfg.btnNormal);
            if (auto* lbl = m_backBtn->GetLabel()) lbl->SetColor(cfg.textNormal);
        }
    }

    void ControlsMenu::OnBackClicked()
    {
        Engine::SoundManager::PlaySFX("ui_click");
        Hide();
    }
}
