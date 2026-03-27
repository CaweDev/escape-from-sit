#include "DebugMenu.hpp"
#include <iomanip>
#include <sstream>
#include "Engine.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include "Game/AccessCard/AccessCard.hpp"
#include "Game/Player/Player.hpp"
#include "Game/Puzzle/PuzzleManager.h"
#include "Game/Scenes/FPSScene/UI/PlayerHUD.hpp"

namespace
{
    struct DebugMenuConfig : public Engine::IConfig
    {
        float referenceWidth = 1280.0f;
        float referenceHeight = 720.0f;
        int canvasSortOrder = 9999;

        float cameraRefreshInterval = 0.5f;
        int cameraPositionPrecision = 2;

        Engine::Color renderLimitColor = Engine::Color(0.0f, 1.0f, 0.0f, 1.0f);
        Engine::Color panelColor = Engine::Color(0.1f, 0.1f, 0.1f, 0.85f);
        Engine::Color textPrimary = Engine::Color(0.9f, 0.9f, 0.9f, 1.0f);
        Engine::Color textSecondary = Engine::Color(0.8f, 0.8f, 0.8f, 1.0f);
        Engine::Color textHint = Engine::Color(0.7f, 0.7f, 0.7f, 1.0f);
        Engine::Color cheatTitleColor = Engine::Color(0.9f, 0.9f, 0.5f, 1.0f);

        Engine::Color buttonNormal = Engine::Color(0.3f, 0.3f, 0.3f, 1.0f);
        Engine::Color buttonHighlight = Engine::Color(0.4f, 0.4f, 0.4f, 1.0f);
        Engine::Color buttonPressed = Engine::Color(0.5f, 0.5f, 0.5f, 1.0f);
        Engine::Color buttonOn = Engine::Color(0.1f, 0.5f, 0.1f, 1.0f);
        Engine::Color buttonOnNormal = Engine::Color(0.2f, 0.6f, 0.2f, 1.0f);
        Engine::Color buttonOnHighlight = Engine::Color(0.3f, 0.7f, 0.3f, 1.0f);
        Engine::Color buttonOnPressed = Engine::Color(0.4f, 0.8f, 0.4f, 1.0f);

        float buttonPixelsPerUnitMultiplier = 2.0f;
        float titleFontSize = 2.5f;
        float instructionFontSize = 1.1f;
        float cameraPosFontSize = 1.5f;
        float buttonFontSize = 1.2f;
        float longButtonFontSize = 1.1f;
        float cheatTitleFontSize = 1.3f;

        std::string buttonSpritePath = "Assets/UI/UI Kit/Panel/panel-027.png";

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
            referenceWidth = j.get("referenceWidth", referenceWidth).asFloat();
            referenceHeight = j.get("referenceHeight", referenceHeight).asFloat();
            canvasSortOrder = j.get("canvasSortOrder", canvasSortOrder).asInt();

            cameraRefreshInterval = j.get("cameraRefreshInterval", cameraRefreshInterval).asFloat();
            cameraPositionPrecision = j.get("cameraPositionPrecision", cameraPositionPrecision).asInt();

            renderLimitColor = ParseColor(j, "renderLimitColor", renderLimitColor);
            panelColor = ParseColor(j, "panelColor", panelColor);
            textPrimary = ParseColor(j, "textPrimary", textPrimary);
            textSecondary = ParseColor(j, "textSecondary", textSecondary);
            textHint = ParseColor(j, "textHint", textHint);
            cheatTitleColor = ParseColor(j, "cheatTitleColor", cheatTitleColor);

            buttonNormal = ParseColor(j, "buttonNormal", buttonNormal);
            buttonHighlight = ParseColor(j, "buttonHighlight", buttonHighlight);
            buttonPressed = ParseColor(j, "buttonPressed", buttonPressed);
            buttonOn = ParseColor(j, "buttonOn", buttonOn);
            buttonOnNormal = ParseColor(j, "buttonOnNormal", buttonOnNormal);
            buttonOnHighlight = ParseColor(j, "buttonOnHighlight", buttonOnHighlight);
            buttonOnPressed = ParseColor(j, "buttonOnPressed", buttonOnPressed);

            buttonPixelsPerUnitMultiplier = j.get("buttonPixelsPerUnitMultiplier", buttonPixelsPerUnitMultiplier).asFloat();
            titleFontSize = j.get("titleFontSize", titleFontSize).asFloat();
            instructionFontSize = j.get("instructionFontSize", instructionFontSize).asFloat();
            cameraPosFontSize = j.get("cameraPosFontSize", cameraPosFontSize).asFloat();
            buttonFontSize = j.get("buttonFontSize", buttonFontSize).asFloat();
            longButtonFontSize = j.get("longButtonFontSize", longButtonFontSize).asFloat();
            cheatTitleFontSize = j.get("cheatTitleFontSize", cheatTitleFontSize).asFloat();

            buttonSpritePath = j.get("buttonSpritePath", buttonSpritePath).asString();
        }
    };

    DebugMenuConfig g_debugMenuConfig;
    bool g_debugMenuConfigLoaded = false;

    const DebugMenuConfig& GetDebugMenuConfig()
    {
        if (!g_debugMenuConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/debug_menu.json", g_debugMenuConfig);
            if (g_debugMenuConfig.referenceWidth < 1.0f) g_debugMenuConfig.referenceWidth = 1.0f;
            if (g_debugMenuConfig.referenceHeight < 1.0f) g_debugMenuConfig.referenceHeight = 1.0f;
            if (g_debugMenuConfig.cameraRefreshInterval < 0.01f) g_debugMenuConfig.cameraRefreshInterval = 0.01f;
            if (g_debugMenuConfig.cameraPositionPrecision < 0) g_debugMenuConfig.cameraPositionPrecision = 0;
            g_debugMenuConfigLoaded = true;
        }
        return g_debugMenuConfig;
    }
}

namespace Game
{
    void DebugMenu::Initialize()
    {
        BuildUI();
    }

    void DebugMenu::Update()
    {
        const auto& cfg = GetDebugMenuConfig();
        if (m_showRenderLimit)
        {
            if (Engine::Camera::main)
            {
                // If it's not our debug green color, save it, then override it
                Engine::Color currentCamColor = Engine::Camera::main->backgroundColor;
                if (currentCamColor.r != cfg.renderLimitColor.r || currentCamColor.g != cfg.renderLimitColor.g || currentCamColor.b != cfg.renderLimitColor.b)
                {
                    m_originalCameraColor = currentCamColor;
                }
                Engine::Camera::main->backgroundColor = cfg.renderLimitColor;
            }
        }

        // Reset cheat flags when leaving FPSScene so they don't persist
        {
            Engine::Scene* active = Engine::SceneManager::GetActiveScene();
            bool inFPSScene = active && active->name == "FPS";
            if (!inFPSScene)
            {
                Game::Player::cheatInfiniteSpeed = false;
                Game::Player::cheatCatchImmune   = false;
            }
        }

        if (!m_isVisible) return;

        // Sync cheat panel visibility while the menu is open (scene may change at runtime)
        {
            Engine::Scene* active = Engine::SceneManager::GetActiveScene();
            bool inFPSScene = active && active->name == "FPS";
            if (m_cheatPanel) 
                m_cheatPanel->visible = inFPSScene;
        }

        // Handle Ctrl-key to unlock cursor while menu is visible
        bool isCtrlHeld = Engine::Input::GetKey(Engine::KeyCode::LeftControl) || Engine::Input::GetKey(Engine::KeyCode::RightControl);
        if (isCtrlHeld && !m_isCursorUnlocked)
        {
            Engine::Input::RegisterCursorUnlock("DebugMenu");
            m_isCursorUnlocked = true;
        }
        else if (!isCtrlHeld && m_isCursorUnlocked)
        {
            Engine::Input::UnregisterCursorUnlock("DebugMenu");
            m_isCursorUnlocked = false;
        }
        
        m_cameraPositionUpdateTimer += Engine::Time::unscaledDeltaTime;
        if (m_cameraPositionUpdateTimer >= cfg.cameraRefreshInterval)
        {
            m_cameraPositionUpdateTimer = 0.0f;
            if (m_cameraPositionText)
            {
                std::stringstream ss;
                if (Engine::Camera::main && Engine::Camera::main->transform)
                {
                    const auto& camPos = Engine::Camera::main->transform->position;
                    ss << "Cam Pos: ("
                       << std::fixed << std::setprecision(cfg.cameraPositionPrecision)
                       << camPos.x << ", " << camPos.y << ", " << camPos.z << ")";
                }
                else
                {
                    ss << "Cam Pos: (N/A)";
                }
                m_cameraPositionText->SetText(ss.str());
            }
        }
    }

    void DebugMenu::Shutdown()
    {
        if (m_isVisible)
        {
            Hide();
        }
        
        if (m_canvas)
        {
            UI::UIManager::GetInstance()->DestroyCanvas(m_canvas);
            m_canvas = nullptr;
            m_panel = nullptr;
            m_titleText = nullptr;
            m_cameraPositionText = nullptr;
            m_renderLimitBtn = nullptr;
            m_showCollidersBtn = nullptr;
            m_cheatPanel = nullptr;
            m_infiniteSpeedBtn = nullptr;
            m_catchImmuneBtn = nullptr;
            m_getAllAccessCardsBtn = nullptr;
            m_collectAllPuzzlesBtn = nullptr;
            m_instructionText = nullptr;
        }
    }

    void DebugMenu::Show()
    {
        if (m_isVisible) return;
        
        m_isVisible = true;

        if (m_panel) 
        {
            m_panel->visible = true;
        }

        // Sync cheat panel visibility at the moment the menu opens
        bool inFPSScene = false;
        {
            Engine::Scene* active = Engine::SceneManager::GetActiveScene();
            if (active && active->name == "FPSScene") inFPSScene = true;
        }
        if (m_cheatPanel) m_cheatPanel->visible = inFPSScene;
    }

    void DebugMenu::Hide()
    {
        if (!m_isVisible) return;

        m_isVisible = false;
        
        if (m_panel) 
        {
            m_panel->visible = false;
        }

        if (m_isCursorUnlocked)
        {
            Engine::Input::UnregisterCursorUnlock("DebugMenu");
            m_isCursorUnlocked = false;
        }
    }

    void DebugMenu::Toggle()
    {
        if (m_isVisible)
        {
            Hide();
        }
        else
        {
            Show();
        }
    }

    void DebugMenu::BuildUI()
    {
        const auto& cfg = GetDebugMenuConfig();
        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        m_canvas->SetReferenceResolution(cfg.referenceWidth, cfg.referenceHeight);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);
        m_canvas->SetSortOrder(cfg.canvasSortOrder); // Highest priority to render over everything
        
        m_panel = m_canvas->CreateChild<UI::Panel>();
        m_panel->name = "DebugMenuPanel";
        m_panel->SetAnchors({ 0.0f, 0.0f }, { 0.3f, 1.0f }); // Left side panel
        m_panel->SetAnchoredPosition({ 0.0f, 0.0f });
        m_panel->SetSizeDelta({ 0.0f, 0.0f });
        m_panel->SetBackgroundColor(cfg.panelColor);
        m_panel->SetRenderOrder(0);
        m_panel->visible = false; // Start hidden
        
        m_titleText = m_panel->CreateChild<UI::Text>();
        m_titleText->name = "DebugMenuTitle";
        m_titleText->SetAnchors({ 0.1f, 0.9f }, { 0.9f, 0.98f });
        m_titleText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_titleText->SetSizeDelta({ 0.0f, 0.0f });
        m_titleText->SetText("DEBUG MENU");
        m_titleText->SetFontSize(cfg.titleFontSize);
        m_titleText->SetColor(cfg.textPrimary);
        m_titleText->SetRenderOrder(1);

        m_instructionText = m_panel->CreateChild<UI::Text>();
        m_instructionText->name = "DebugMenuInstruction";
        m_instructionText->SetAnchors({ 0.1f, 0.86f }, { 0.9f, 0.91f });
        m_instructionText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_instructionText->SetSizeDelta({ 0.0f, 0.0f });
        m_instructionText->SetText("[Hold CTRL for Mouse]");
        m_instructionText->SetFontSize(cfg.instructionFontSize);
        m_instructionText->SetColor(cfg.textHint);
        m_instructionText->SetRenderOrder(1);

        m_cameraPositionText = m_panel->CreateChild<UI::Text>();
        m_cameraPositionText->name = "DebugMenuCameraPosition";
        m_cameraPositionText->SetAnchors({ 0.1f, 0.78f }, { 0.9f, 0.83f });
        m_cameraPositionText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_cameraPositionText->SetSizeDelta({ 0.0f, 0.0f });
        m_cameraPositionText->SetText("Cam Pos: (--, --, --)");
        m_cameraPositionText->SetFontSize(cfg.cameraPosFontSize);
        m_cameraPositionText->SetColor(cfg.textSecondary);
        m_cameraPositionText->SetRenderOrder(1);

        m_renderLimitBtn = m_panel->CreateChild<UI::Button>();
        m_renderLimitBtn->name = "RenderLimitBtn";
        m_renderLimitBtn->SetAnchors({ 0.1f, 0.68f }, { 0.9f, 0.76f });
        m_renderLimitBtn->SetAnchoredPosition({ 0.0f, 0.0f });
        m_renderLimitBtn->SetSizeDelta({ 0.0f, 0.0f });
        m_renderLimitBtn->SetBackgroundColor(cfg.buttonNormal);
        m_renderLimitBtn->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.buttonSpritePath.c_str()), true);
        if (m_renderLimitBtn->GetBackground()) m_renderLimitBtn->GetBackground()->SetPixelsPerUnitMultiplier(cfg.buttonPixelsPerUnitMultiplier);
        
        UI::ColorBlock btnColors;
        btnColors.normalColor = cfg.buttonNormal;
        btnColors.highlightedColor = cfg.buttonHighlight;
        btnColors.pressedColor = cfg.buttonPressed;
        m_renderLimitBtn->SetColors(btnColors);

        m_renderLimitBtn->SetText("Show Render Limit: OFF");
        if (m_renderLimitBtn->GetLabel())
        {
            m_renderLimitBtn->GetLabel()->SetFontSize(cfg.buttonFontSize);
            m_renderLimitBtn->GetLabel()->SetColor(cfg.textPrimary);
        }
        m_renderLimitBtn->SetRenderOrder(2);
        
        m_showCollidersBtn = m_panel->CreateChild<UI::Button>();
        m_showCollidersBtn->name = "ShowCollidersBtn";
        m_showCollidersBtn->SetAnchors({ 0.1f, 0.57f }, { 0.9f, 0.65f });
        m_showCollidersBtn->SetAnchoredPosition({ 0.0f, 0.0f });
        m_showCollidersBtn->SetSizeDelta({ 0.0f, 0.0f });
        m_showCollidersBtn->SetBackgroundColor(cfg.buttonNormal);
        m_showCollidersBtn->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.buttonSpritePath.c_str()), true);
        if (m_showCollidersBtn->GetBackground()) m_showCollidersBtn->GetBackground()->SetPixelsPerUnitMultiplier(cfg.buttonPixelsPerUnitMultiplier);
        m_showCollidersBtn->SetColors(btnColors);

        bool initDebugDraw = Engine::Physics2D::debugDrawEnabled;
        m_showCollidersBtn->SetText(initDebugDraw ? "Show Colliders: ON" : "Show Colliders: OFF");
        if (initDebugDraw) m_showCollidersBtn->SetBackgroundColor(cfg.buttonOn);
        
        if (m_showCollidersBtn->GetLabel())
        {
            m_showCollidersBtn->GetLabel()->SetFontSize(cfg.buttonFontSize);
            m_showCollidersBtn->GetLabel()->SetColor(cfg.textPrimary);
        }
        m_showCollidersBtn->SetRenderOrder(2);

        // ---- FPSScene-only cheat panel ----
        // Wrapping buttons in their own panel so visible toggling works in the hierarchy.
        m_cheatPanel = m_panel->CreateChild<UI::Panel>();
        m_cheatPanel->name = "CheatPanel";
        m_cheatPanel->SetAnchors({ 0.0f, 0.20f }, { 1.0f, 0.5f });
        m_cheatPanel->SetAnchoredPosition({ 0.0f, 0.0f });
        m_cheatPanel->SetSizeDelta({ 0.0f, 0.0f });
        m_cheatPanel->SetBackgroundColor({ 0.0f, 0.0f, 0.0f, 0.0f }); // transparent
        m_cheatPanel->SetRenderOrder(1);
        m_cheatPanel->visible = false;

        // ---- Cheat panel title ----
        UI::Text* cheatTitle = m_cheatPanel->CreateChild<UI::Text>();
        cheatTitle->name = "CheatPanelTitle";
        cheatTitle->SetAnchors({ 0.1f, 0.84f }, { 0.9f, 0.98f });
        cheatTitle->SetAnchoredPosition({ 0.0f, 0.0f });
        cheatTitle->SetSizeDelta({ 0.0f, 0.0f });
        cheatTitle->SetText("Game Scene");
        cheatTitle->SetFontSize(cfg.cheatTitleFontSize);
        cheatTitle->SetColor(cfg.cheatTitleColor);
        cheatTitle->SetAnchor(UI::TextAnchor::MiddleCenter);
        cheatTitle->SetRenderOrder(2);

        // ---- Cheat buttons are children of m_cheatPanel ----
        m_infiniteSpeedBtn = m_cheatPanel->CreateChild<UI::Button>();
        m_infiniteSpeedBtn->name = "InfiniteSpeedBtn";
        m_infiniteSpeedBtn->SetAnchors({ 0.1f, 0.60f }, { 0.9f, 0.80f });
        m_infiniteSpeedBtn->SetAnchoredPosition({ 0.0f, 0.0f });
        m_infiniteSpeedBtn->SetSizeDelta({ 0.0f, 0.0f });
        m_infiniteSpeedBtn->SetBackgroundColor(cfg.buttonNormal);
        m_infiniteSpeedBtn->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.buttonSpritePath.c_str()), true);
        if (m_infiniteSpeedBtn->GetBackground()) m_infiniteSpeedBtn->GetBackground()->SetPixelsPerUnitMultiplier(cfg.buttonPixelsPerUnitMultiplier);
        m_infiniteSpeedBtn->SetColors(btnColors);
        m_infiniteSpeedBtn->SetText("Infinite Speed: OFF");
        if (m_infiniteSpeedBtn->GetLabel())
        {
            m_infiniteSpeedBtn->GetLabel()->SetFontSize(cfg.buttonFontSize);
            m_infiniteSpeedBtn->GetLabel()->SetColor(cfg.textPrimary);
        }
        m_infiniteSpeedBtn->SetRenderOrder(2);

        m_catchImmuneBtn = m_cheatPanel->CreateChild<UI::Button>();
        m_catchImmuneBtn->name = "CatchImmuneBtn";
        m_catchImmuneBtn->SetAnchors({ 0.1f, 0.38f }, { 0.9f, 0.58f });
        m_catchImmuneBtn->SetAnchoredPosition({ 0.0f, 0.0f });
        m_catchImmuneBtn->SetSizeDelta({ 0.0f, 0.0f });
        m_catchImmuneBtn->SetBackgroundColor(cfg.buttonNormal);
        m_catchImmuneBtn->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.buttonSpritePath.c_str()), true);
        if (m_catchImmuneBtn->GetBackground()) m_catchImmuneBtn->GetBackground()->SetPixelsPerUnitMultiplier(cfg.buttonPixelsPerUnitMultiplier);
        m_catchImmuneBtn->SetColors(btnColors);
        m_catchImmuneBtn->SetText("Catch Immune: OFF");
        if (m_catchImmuneBtn->GetLabel())
        {
            m_catchImmuneBtn->GetLabel()->SetFontSize(cfg.buttonFontSize);
            m_catchImmuneBtn->GetLabel()->SetColor(cfg.textPrimary);
        }
        m_catchImmuneBtn->SetRenderOrder(2);

        m_getAllAccessCardsBtn = m_cheatPanel->CreateChild<UI::Button>();
        m_getAllAccessCardsBtn->name = "GetAllAccessCardsBtn";
        m_getAllAccessCardsBtn->SetAnchors({ 0.1f, 0.16f }, { 0.9f, 0.36f });
        m_getAllAccessCardsBtn->SetAnchoredPosition({ 0.0f, 0.0f });
        m_getAllAccessCardsBtn->SetSizeDelta({ 0.0f, 0.0f });
        m_getAllAccessCardsBtn->SetBackgroundColor(cfg.buttonNormal);
        m_getAllAccessCardsBtn->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.buttonSpritePath.c_str()), true);
        if (m_getAllAccessCardsBtn->GetBackground()) m_getAllAccessCardsBtn->GetBackground()->SetPixelsPerUnitMultiplier(cfg.buttonPixelsPerUnitMultiplier);
        m_getAllAccessCardsBtn->SetColors(btnColors);
        m_getAllAccessCardsBtn->SetText("Get All Access Cards");
        if (m_getAllAccessCardsBtn->GetLabel())
        {
            m_getAllAccessCardsBtn->GetLabel()->SetFontSize(cfg.longButtonFontSize);
            m_getAllAccessCardsBtn->GetLabel()->SetColor(cfg.textPrimary);
        }
        m_getAllAccessCardsBtn->SetRenderOrder(2);

        m_collectAllPuzzlesBtn = m_cheatPanel->CreateChild<UI::Button>();
        m_collectAllPuzzlesBtn->name = "CollectAllPuzzlesBtn";
        m_collectAllPuzzlesBtn->SetAnchors({ 0.1f, 0.00f }, { 0.9f, 0.14f });
        m_collectAllPuzzlesBtn->SetAnchoredPosition({ 0.0f, 0.0f });
        m_collectAllPuzzlesBtn->SetSizeDelta({ 0.0f, 0.0f });
        m_collectAllPuzzlesBtn->SetBackgroundColor(cfg.buttonNormal);
        m_collectAllPuzzlesBtn->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.buttonSpritePath.c_str()), true);
        if (m_collectAllPuzzlesBtn->GetBackground()) m_collectAllPuzzlesBtn->GetBackground()->SetPixelsPerUnitMultiplier(cfg.buttonPixelsPerUnitMultiplier);
        m_collectAllPuzzlesBtn->SetColors(btnColors);
        m_collectAllPuzzlesBtn->SetText("Collect All Puzzles");
        if (m_collectAllPuzzlesBtn->GetLabel())
        {
            m_collectAllPuzzlesBtn->GetLabel()->SetFontSize(cfg.longButtonFontSize);
            m_collectAllPuzzlesBtn->GetLabel()->SetColor(cfg.textPrimary);
        }
        m_collectAllPuzzlesBtn->SetRenderOrder(2);

        // ---- Cheat button callbacks ----
        m_infiniteSpeedBtn->onClick = [this]() {
            const auto& cfg = GetDebugMenuConfig();
            Game::Player::cheatInfiniteSpeed = !Game::Player::cheatInfiniteSpeed;
            bool isON = Game::Player::cheatInfiniteSpeed;
            m_infiniteSpeedBtn->SetText(isON ? "Infinite Speed: ON" : "Infinite Speed: OFF");
            m_infiniteSpeedBtn->SetBackgroundColor(isON ? cfg.buttonOn : cfg.buttonNormal);
            UI::ColorBlock c;
            if (isON) { c.normalColor = cfg.buttonOnNormal; c.highlightedColor = cfg.buttonOnHighlight; c.pressedColor = cfg.buttonOnPressed; }
            else      { c.normalColor = cfg.buttonNormal; c.highlightedColor = cfg.buttonHighlight; c.pressedColor = cfg.buttonPressed; }
            m_infiniteSpeedBtn->SetColors(c);
        };

        m_catchImmuneBtn->onClick = [this]() {
            const auto& cfg = GetDebugMenuConfig();
            Game::Player::cheatCatchImmune = !Game::Player::cheatCatchImmune;
            bool isON = Game::Player::cheatCatchImmune;
            m_catchImmuneBtn->SetText(isON ? "Catch Immune: ON" : "Catch Immune: OFF");
            m_catchImmuneBtn->SetBackgroundColor(isON ? cfg.buttonOn : cfg.buttonNormal);
            UI::ColorBlock c;
            if (isON) { c.normalColor = cfg.buttonOnNormal; c.highlightedColor = cfg.buttonOnHighlight; c.pressedColor = cfg.buttonOnPressed; }
            else      { c.normalColor = cfg.buttonNormal; c.highlightedColor = cfg.buttonHighlight; c.pressedColor = cfg.buttonPressed; }
            m_catchImmuneBtn->SetColors(c);
        };

        m_getAllAccessCardsBtn->onClick = [this]() {
            auto* playerObj = Engine::GameObject::Find("Player");
            auto* hud = playerObj ? playerObj->GetComponent<Game::PlayerHUD>() : nullptr;
            Engine::Vector2 collectPos = playerObj && playerObj->transform
                ? Engine::Vector2(playerObj->transform->position.x, playerObj->transform->position.y)
                : Engine::Vector2(0.0f, 0.0f);

            if (hud)
            {
                for (int cardId = 0; cardId < 4; ++cardId)
                {
                    if (!hud->HasAccessCard(cardId))
                    {
                        hud->CollectAccessCard(cardId, collectPos);
                    }
                }
            }

            for (auto* obj : Engine::GameObject::GetAllGameObjects())
            {
                if (!obj || obj->isPendingDestruction) continue;
                if (obj->GetComponent<Game::AccessCard>())
                {
                    obj->active = false;
                }
            }

            m_getAllAccessCardsBtn->SetText("Get All Access Cards (Done)");
        };

        m_collectAllPuzzlesBtn->onClick = [this]() {
            for (auto& book : PuzzleManager::allBooks)
            {
                book.BookCollected = true;
            }

            PuzzleManager::booksCollected = static_cast<int>(PuzzleManager::allBooks.size());
            PuzzleManager::isQuizActive = false;
            PuzzleManager::popupTimer = 0.0f;
            PuzzleManager::currentBookObj = nullptr;

            for (auto* obj : Engine::GameObject::GetAllGameObjects())
            {
                if (!obj || obj->isPendingDestruction) continue;
                if (obj->name == "Book")
                {
                    Engine::GameObject::Destroy(obj);
                }
            }

            PuzzleManager::OpenExitDoor();
            m_collectAllPuzzlesBtn->SetText("Collect All Puzzles (Done)");
        };

        m_renderLimitBtn->onClick = [this]() {
            const auto& cfg = GetDebugMenuConfig();
            m_showRenderLimit = !m_showRenderLimit;
            
            if (m_showRenderLimit)
            {
                m_renderLimitBtn->SetText("Show Render Limit: ON");
                m_renderLimitBtn->SetBackgroundColor(cfg.buttonOn); // Greenish tint
                
                UI::ColorBlock activeColors;
                activeColors.normalColor = cfg.buttonOnNormal;
                activeColors.highlightedColor = cfg.buttonOnHighlight;
                activeColors.pressedColor = cfg.buttonOnPressed;
                m_renderLimitBtn->SetColors(activeColors);
            }
            else
            {
                if (Engine::Camera::main)
                {
                    Engine::Camera::main->backgroundColor = m_originalCameraColor;
                }
                
                m_renderLimitBtn->SetText("Show Render Limit: OFF");
                m_renderLimitBtn->SetBackgroundColor(cfg.buttonNormal); // Default tint
                
                UI::ColorBlock inactiveColors;
                inactiveColors.normalColor = cfg.buttonNormal;
                inactiveColors.highlightedColor = cfg.buttonHighlight;
                inactiveColors.pressedColor = cfg.buttonPressed;
                m_renderLimitBtn->SetColors(inactiveColors);
            }
        };
        
        m_showCollidersBtn->onClick = [this]() {
            const auto& cfg = GetDebugMenuConfig();
            Engine::Physics2D::debugDrawEnabled = !Engine::Physics2D::debugDrawEnabled;
            bool isON = Engine::Physics2D::debugDrawEnabled;
            
            m_showCollidersBtn->SetText(isON ? "Show Colliders: ON" : "Show Colliders: OFF");
            m_showCollidersBtn->SetBackgroundColor(isON ? cfg.buttonOn : cfg.buttonNormal);
            
            UI::ColorBlock colors;
            if (isON) {
                colors.normalColor = cfg.buttonOnNormal;
                colors.highlightedColor = cfg.buttonOnHighlight;
                colors.pressedColor = cfg.buttonOnPressed;
            } else {
                colors.normalColor = cfg.buttonNormal;
                colors.highlightedColor = cfg.buttonHighlight;
                colors.pressedColor = cfg.buttonPressed;
            }
            m_showCollidersBtn->SetColors(colors);
        };
    }
}
