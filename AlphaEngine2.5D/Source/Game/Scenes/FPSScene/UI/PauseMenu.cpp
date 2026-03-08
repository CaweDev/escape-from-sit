/**
 * @file PauseMenu.cpp
 * @brief Implementation of the PauseMenu component
 */

#include "PauseMenu.hpp"
#include <iostream>

namespace Game
{
    // ────────────────────────────────────────────────────────────
    //  Horror palette constants
    // ────────────────────────────────────────────────────────────
    namespace
    {
        // Overlay
        const Engine::Color kOverlayColor       { 0.0f,  0.0f,  0.0f,  0.75f };

        // Menu panel
        const Engine::Color kMenuBgColor        { 0.3f, 0.12f, 0.12f, 0.92f };

        // Title
        const Engine::Color kTitleColor         { 0.8f,  0.15f, 0.10f, 1.0f };

        // Divider
        const Engine::Color kDividerColor       { 0.6f,  0.10f, 0.08f, 0.7f };

        // Button states
        const Engine::Color kBtnNormal          { 0.12f, 0.05f, 0.05f, 0.9f };
        const Engine::Color kBtnHighlighted     { 0.22f, 0.08f, 0.08f, 1.0f };
        const Engine::Color kBtnPressed         { 0.30f, 0.10f, 0.10f, 1.0f };
        const Engine::Color kBtnDisabled        { 0.08f, 0.04f, 0.04f, 0.5f };

        // Text
        const Engine::Color kTextNormal         { 0.6f,  0.6f,  0.6f,  1.0f };
        const Engine::Color kTextSelected       { 1.0f,  1.0f,  1.0f,  1.0f };

        // Layout
        constexpr float kMenuWidth   = 400.0f;
        constexpr float kMenuHeight  = 400.0f;
        constexpr float kBtnWidth    = 280.0f;
        constexpr float kBtnHeight   = 52.0f;
        constexpr float kBtnSpacing  = 16.0f;

        constexpr float kFontSizeBase = 1.8f;
        constexpr float kFontSizeSelectedScale = 1.15f;
        constexpr float kLerpSpeed = 10.0f;
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
                UpdateMenuVisuals();
            }

            if (Engine::Input::GetKeyDown(Engine::KeyCode::S) ||
                Engine::Input::GetKeyDown(Engine::KeyCode::DownArrow))
            {
                m_selectedIndex = (m_selectedIndex + 1) % itemCount;
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
        if (m_isPaused) return;

        m_isPaused = true;
        m_selectedIndex = 0;

        if (m_overlay)  m_overlay->visible  = true;

        // Pause game
        Engine::Application::RegisterPause("PauseMenu");

        // Unlock cursor so the player can click buttons
        if (Engine::Input::IsCursorLocked())
        {
            Engine::Input::UnregisterCursorLock("FPSController");
        }

        UpdateMenuVisuals();
    }

    void PauseMenu::Hide()
    {
        if (!m_isPaused) return;

        m_isPaused = false;

        if (m_overlay) m_overlay->visible = false;

        // Unpause game
        Engine::Application::UnregisterPause("PauseMenu");

        // Re-lock cursor for FPS controls
        Engine::Input::RegisterCursorLock("FPSController");
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
        // --- Canvas ---
        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        m_canvas->SetReferenceResolution(1280.0f, 720.0f);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);
        m_canvas->SetSortOrder(500);  // Above HUD (100), below end-panel (1000)

        // --- Full-screen overlay ---
        m_overlay = m_canvas->CreateChild<UI::Panel>();
        m_overlay->name = "PauseOverlay";
        m_overlay->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_overlay->SetAnchoredPosition({ 0.0f, 0.0f });
        m_overlay->SetSizeDelta({ 0.0f, 0.0f });
        m_overlay->SetBackgroundColor(kOverlayColor);
        m_overlay->SetRenderOrder(0);
        m_overlay->visible = false;  // Start hidden

        // --- Centered menu panel ---
        m_menuPanel = m_overlay->CreateChild<UI::Panel>();
        m_menuPanel->name = "PauseMenuPanel";
        m_menuPanel->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
        m_menuPanel->SetPivot({ 0.5f, 0.5f });
        m_menuPanel->SetAnchoredPosition({ 0.0f, 0.0f });
        m_menuPanel->SetSizeDelta({ kMenuWidth, kMenuHeight });
        m_menuPanel->SetBackgroundSprite(Engine::AssetManager::GetTexture("Assets/UI/UI Kit/Panel/panel-000.png"));
        m_menuPanel->GetBackground()->SetImageType(UI::ImageType::Sliced);
        m_menuPanel->GetBackground()->SetPixelsPerUnitMultiplier(4.0f); // Make the 9-slice borders thicker
        m_menuPanel->SetBackgroundColor(kMenuBgColor);
        m_menuPanel->SetRenderOrder(1);

        // --- Title text: "PAUSED" ---
        m_title = m_menuPanel->CreateChild<UI::Text>();
        m_title->name = "PauseTitle";
        m_title->SetAnchors({ 0.0f, 0.7f }, { 1.0f, 0.95f });
        m_title->SetAnchoredPosition({ 0.0f, 0.0f });
        m_title->SetSizeDelta({ 0.0f, 0.0f });
        m_title->SetText("PAUSED");
        m_title->SetFontSize(3.5f);
        m_title->SetColor(kTitleColor);
        m_title->SetAnchor(UI::TextAnchor::MiddleCenter);
        m_title->SetRenderOrder(2);

        // --- Divider line ---
        m_divider = m_menuPanel->CreateChild<UI::Image>();
        m_divider->name = "PauseDivider";
        m_divider->SetAnchors({ 0.15f, 0.74f }, { 0.85f, 0.74f });
        m_divider->SetPivot({ 0.5f, 0.5f });
        m_divider->SetAnchoredPosition({ 0.0f, 0.0f });
        m_divider->SetSizeDelta({ 0.0f, 2.0f });
        m_divider->SetColor(kDividerColor);
        m_divider->SetRenderOrder(2);

        // --- Button style ---
        UI::ColorBlock btnColors;
        btnColors.normalColor      = kBtnNormal;
        btnColors.highlightedColor = kBtnHighlighted;
        btnColors.pressedColor     = kBtnPressed;
        btnColors.disabledColor    = kBtnDisabled;
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
        const float btnNormH    = kBtnHeight / kMenuHeight;          // button height in anchor space
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
            btn->SetSizeDelta({ kBtnWidth, 0.0f });
            btn->SetColors(btnColors);
            btn->SetBackgroundColor(kBtnNormal);
            btn->SetText(configs[i].label);
            btn->onClick = configs[i].callback;
            btn->SetRenderOrder(3);
            btn->SetBackgroundSprite(Engine::AssetManager::GetTexture("Assets/UI/UI Kit/Panel/panel-027.png"));
            btn->GetBackground()->SetImageType(UI::ImageType::Sliced);
            btn->GetBackground()->SetPixelsPerUnitMultiplier(1.0f); // Make the 9-slice borders thicker

            // Style the label text
            if (auto* label = btn->GetLabel())
            {
                label->SetFontSize(kFontSizeBase);
                label->SetColor(kTextNormal);
            }

            // Track for keyboard nav
            MenuItem item;
            item.button          = btn;
            item.label           = configs[i].label;
            item.baseFontSize    = kFontSizeBase;
            item.currentFontSize = kFontSizeBase;
            item.targetFontSize  = kFontSizeBase;
            m_items.push_back(item);
        }

        // Keep pointers for direct access
        m_resumeBtn   = m_items[0].button;
        m_restartBtn  = m_items[1].button;
        m_mainMenuBtn = m_items[2].button;
    }

    // ────────────────────────────────────────────────────────────
    //  Visual Updates
    // ────────────────────────────────────────────────────────────

    void PauseMenu::UpdateMenuVisuals()
    {
        for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
        {
            bool selected = (i == m_selectedIndex);
            auto* label = m_items[i].button->GetLabel();
            if (!label) continue;

            if (selected)
            {
                label->SetText("> " + m_items[i].label);
                label->SetColor(kTextSelected);
                m_items[i].targetFontSize = m_items[i].baseFontSize * kFontSizeSelectedScale;
            }
            else
            {
                label->SetText(m_items[i].label);
                label->SetColor(kTextNormal);
                m_items[i].targetFontSize = m_items[i].baseFontSize;
            }
        }
    }

    void PauseMenu::UpdateMenuAnimations(float dt)
    {
        for (auto& item : m_items)
        {
            item.currentFontSize += (item.targetFontSize - item.currentFontSize) * dt * kLerpSpeed;

            if (auto* label = item.button->GetLabel())
            {
                label->SetFontSize(item.currentFontSize);
            }
        }
    }

    void PauseMenu::SelectMenuItem(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_items.size())) return;

        if (m_items[index].button && m_items[index].button->onClick)
        {
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
                Engine::Input::RegisterCursorLock("FPSController");
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
                Engine::SceneManager::LoadWithLoadingScreen("MainMenu");
            }
        );
    }

} // namespace Game
