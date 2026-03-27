#include "DebugMenu.hpp"
#include <iomanip>
#include <sstream>
#include "Engine.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Game/AccessCard/AccessCard.hpp"
#include "Game/Player/Player.hpp"
#include "Game/Puzzle/PuzzleManager.h"
#include "Game/Scenes/FPSScene/UI/PlayerHUD.hpp"

namespace Game
{
    void DebugMenu::Initialize()
    {
        BuildUI();
    }

    void DebugMenu::Update()
    {
        if (m_showRenderLimit)
        {
            if (Engine::Camera::main)
            {
                // If it's not our debug green color, save it, then override it
                Engine::Color currentCamColor = Engine::Camera::main->backgroundColor;
                if (currentCamColor.r != 0.0f || currentCamColor.g != 1.0f || currentCamColor.b != 0.0f)
                {
                    m_originalCameraColor = currentCamColor;
                }
                Engine::Camera::main->backgroundColor = { 0.0f, 1.0f, 0.0f, 1.0f }; // Green
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
        if (m_cameraPositionUpdateTimer >= 0.5f)
        {
            m_cameraPositionUpdateTimer = 0.0f;
            if (m_cameraPositionText)
            {
                std::stringstream ss;
                if (Engine::Camera::main && Engine::Camera::main->transform)
                {
                    const auto& camPos = Engine::Camera::main->transform->position;
                    ss << "Cam Pos: ("
                       << std::fixed << std::setprecision(2)
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
        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        m_canvas->SetReferenceResolution(1280.0f, 720.0f);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);
        m_canvas->SetSortOrder(9999); // Highest priority to render over everything
        
        m_panel = m_canvas->CreateChild<UI::Panel>();
        m_panel->name = "DebugMenuPanel";
        m_panel->SetAnchors({ 0.0f, 0.0f }, { 0.3f, 1.0f }); // Left side panel
        m_panel->SetAnchoredPosition({ 0.0f, 0.0f });
        m_panel->SetSizeDelta({ 0.0f, 0.0f });
        m_panel->SetBackgroundColor({ 0.1f, 0.1f, 0.1f, 0.85f });
        m_panel->SetRenderOrder(0);
        m_panel->visible = false; // Start hidden
        
        m_titleText = m_panel->CreateChild<UI::Text>();
        m_titleText->name = "DebugMenuTitle";
        m_titleText->SetAnchors({ 0.1f, 0.9f }, { 0.9f, 0.98f });
        m_titleText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_titleText->SetSizeDelta({ 0.0f, 0.0f });
        m_titleText->SetText("DEBUG MENU");
        m_titleText->SetFontSize(2.5f);
        m_titleText->SetColor({ 0.9f, 0.9f, 0.9f, 1.0f });
        m_titleText->SetRenderOrder(1);

        m_instructionText = m_panel->CreateChild<UI::Text>();
        m_instructionText->name = "DebugMenuInstruction";
        m_instructionText->SetAnchors({ 0.1f, 0.86f }, { 0.9f, 0.91f });
        m_instructionText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_instructionText->SetSizeDelta({ 0.0f, 0.0f });
        m_instructionText->SetText("[Hold CTRL for Mouse]");
        m_instructionText->SetFontSize(1.1f);
        m_instructionText->SetColor({ 0.7f, 0.7f, 0.7f, 1.0f });
        m_instructionText->SetRenderOrder(1);

        m_cameraPositionText = m_panel->CreateChild<UI::Text>();
        m_cameraPositionText->name = "DebugMenuCameraPosition";
        m_cameraPositionText->SetAnchors({ 0.1f, 0.78f }, { 0.9f, 0.83f });
        m_cameraPositionText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_cameraPositionText->SetSizeDelta({ 0.0f, 0.0f });
        m_cameraPositionText->SetText("Cam Pos: (--, --, --)");
        m_cameraPositionText->SetFontSize(1.5f);
        m_cameraPositionText->SetColor({ 0.8f, 0.8f, 0.8f, 1.0f });
        m_cameraPositionText->SetRenderOrder(1);

        m_renderLimitBtn = m_panel->CreateChild<UI::Button>();
        m_renderLimitBtn->name = "RenderLimitBtn";
        m_renderLimitBtn->SetAnchors({ 0.1f, 0.68f }, { 0.9f, 0.76f });
        m_renderLimitBtn->SetAnchoredPosition({ 0.0f, 0.0f });
        m_renderLimitBtn->SetSizeDelta({ 0.0f, 0.0f });
        m_renderLimitBtn->SetBackgroundColor({ 0.3f, 0.3f, 0.3f, 1.0f });
        m_renderLimitBtn->SetBackgroundSprite(Engine::AssetManager::GetTexture("Assets/UI/UI Kit/Panel/panel-027.png"), true);
        if (m_renderLimitBtn->GetBackground()) m_renderLimitBtn->GetBackground()->SetPixelsPerUnitMultiplier(2.0f);
        
        UI::ColorBlock btnColors;
        btnColors.normalColor = { 0.3f, 0.3f, 0.3f, 1.0f };
        btnColors.highlightedColor = { 0.4f, 0.4f, 0.4f, 1.0f };
        btnColors.pressedColor = { 0.5f, 0.5f, 0.5f, 1.0f };
        m_renderLimitBtn->SetColors(btnColors);

        m_renderLimitBtn->SetText("Show Render Limit: OFF");
        if (m_renderLimitBtn->GetLabel())
        {
            m_renderLimitBtn->GetLabel()->SetFontSize(1.2f);
            m_renderLimitBtn->GetLabel()->SetColor({ 0.9f, 0.9f, 0.9f, 1.0f });
        }
        m_renderLimitBtn->SetRenderOrder(2);
        
        m_showCollidersBtn = m_panel->CreateChild<UI::Button>();
        m_showCollidersBtn->name = "ShowCollidersBtn";
        m_showCollidersBtn->SetAnchors({ 0.1f, 0.57f }, { 0.9f, 0.65f });
        m_showCollidersBtn->SetAnchoredPosition({ 0.0f, 0.0f });
        m_showCollidersBtn->SetSizeDelta({ 0.0f, 0.0f });
        m_showCollidersBtn->SetBackgroundColor({ 0.3f, 0.3f, 0.3f, 1.0f });
        m_showCollidersBtn->SetBackgroundSprite(Engine::AssetManager::GetTexture("Assets/UI/UI Kit/Panel/panel-027.png"), true);
        if (m_showCollidersBtn->GetBackground()) m_showCollidersBtn->GetBackground()->SetPixelsPerUnitMultiplier(2.0f);
        m_showCollidersBtn->SetColors(btnColors);

        bool initDebugDraw = Engine::Physics2D::debugDrawEnabled;
        m_showCollidersBtn->SetText(initDebugDraw ? "Show Colliders: ON" : "Show Colliders: OFF");
        if (initDebugDraw) m_showCollidersBtn->SetBackgroundColor({ 0.1f, 0.5f, 0.1f, 1.0f });
        
        if (m_showCollidersBtn->GetLabel())
        {
            m_showCollidersBtn->GetLabel()->SetFontSize(1.2f);
            m_showCollidersBtn->GetLabel()->SetColor({ 0.9f, 0.9f, 0.9f, 1.0f });
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
        cheatTitle->SetFontSize(1.3f);
        cheatTitle->SetColor({ 0.9f, 0.9f, 0.5f, 1.0f });
        cheatTitle->SetAnchor(UI::TextAnchor::MiddleCenter);
        cheatTitle->SetRenderOrder(2);

        // ---- Cheat buttons are children of m_cheatPanel ----
        m_infiniteSpeedBtn = m_cheatPanel->CreateChild<UI::Button>();
        m_infiniteSpeedBtn->name = "InfiniteSpeedBtn";
        m_infiniteSpeedBtn->SetAnchors({ 0.1f, 0.60f }, { 0.9f, 0.80f });
        m_infiniteSpeedBtn->SetAnchoredPosition({ 0.0f, 0.0f });
        m_infiniteSpeedBtn->SetSizeDelta({ 0.0f, 0.0f });
        m_infiniteSpeedBtn->SetBackgroundColor({ 0.3f, 0.3f, 0.3f, 1.0f });
        m_infiniteSpeedBtn->SetBackgroundSprite(Engine::AssetManager::GetTexture("Assets/UI/UI Kit/Panel/panel-027.png"), true);
        if (m_infiniteSpeedBtn->GetBackground()) m_infiniteSpeedBtn->GetBackground()->SetPixelsPerUnitMultiplier(2.0f);
        m_infiniteSpeedBtn->SetColors(btnColors);
        m_infiniteSpeedBtn->SetText("Infinite Speed: OFF");
        if (m_infiniteSpeedBtn->GetLabel())
        {
            m_infiniteSpeedBtn->GetLabel()->SetFontSize(1.2f);
            m_infiniteSpeedBtn->GetLabel()->SetColor({ 0.9f, 0.9f, 0.9f, 1.0f });
        }
        m_infiniteSpeedBtn->SetRenderOrder(2);

        m_catchImmuneBtn = m_cheatPanel->CreateChild<UI::Button>();
        m_catchImmuneBtn->name = "CatchImmuneBtn";
        m_catchImmuneBtn->SetAnchors({ 0.1f, 0.38f }, { 0.9f, 0.58f });
        m_catchImmuneBtn->SetAnchoredPosition({ 0.0f, 0.0f });
        m_catchImmuneBtn->SetSizeDelta({ 0.0f, 0.0f });
        m_catchImmuneBtn->SetBackgroundColor({ 0.3f, 0.3f, 0.3f, 1.0f });
        m_catchImmuneBtn->SetBackgroundSprite(Engine::AssetManager::GetTexture("Assets/UI/UI Kit/Panel/panel-027.png"), true);
        if (m_catchImmuneBtn->GetBackground()) m_catchImmuneBtn->GetBackground()->SetPixelsPerUnitMultiplier(2.0f);
        m_catchImmuneBtn->SetColors(btnColors);
        m_catchImmuneBtn->SetText("Catch Immune: OFF");
        if (m_catchImmuneBtn->GetLabel())
        {
            m_catchImmuneBtn->GetLabel()->SetFontSize(1.2f);
            m_catchImmuneBtn->GetLabel()->SetColor({ 0.9f, 0.9f, 0.9f, 1.0f });
        }
        m_catchImmuneBtn->SetRenderOrder(2);

        m_getAllAccessCardsBtn = m_cheatPanel->CreateChild<UI::Button>();
        m_getAllAccessCardsBtn->name = "GetAllAccessCardsBtn";
        m_getAllAccessCardsBtn->SetAnchors({ 0.1f, 0.16f }, { 0.9f, 0.36f });
        m_getAllAccessCardsBtn->SetAnchoredPosition({ 0.0f, 0.0f });
        m_getAllAccessCardsBtn->SetSizeDelta({ 0.0f, 0.0f });
        m_getAllAccessCardsBtn->SetBackgroundColor({ 0.3f, 0.3f, 0.3f, 1.0f });
        m_getAllAccessCardsBtn->SetBackgroundSprite(Engine::AssetManager::GetTexture("Assets/UI/UI Kit/Panel/panel-027.png"), true);
        if (m_getAllAccessCardsBtn->GetBackground()) m_getAllAccessCardsBtn->GetBackground()->SetPixelsPerUnitMultiplier(2.0f);
        m_getAllAccessCardsBtn->SetColors(btnColors);
        m_getAllAccessCardsBtn->SetText("Get All Access Cards");
        if (m_getAllAccessCardsBtn->GetLabel())
        {
            m_getAllAccessCardsBtn->GetLabel()->SetFontSize(1.1f);
            m_getAllAccessCardsBtn->GetLabel()->SetColor({ 0.9f, 0.9f, 0.9f, 1.0f });
        }
        m_getAllAccessCardsBtn->SetRenderOrder(2);

        m_collectAllPuzzlesBtn = m_cheatPanel->CreateChild<UI::Button>();
        m_collectAllPuzzlesBtn->name = "CollectAllPuzzlesBtn";
        m_collectAllPuzzlesBtn->SetAnchors({ 0.1f, 0.00f }, { 0.9f, 0.14f });
        m_collectAllPuzzlesBtn->SetAnchoredPosition({ 0.0f, 0.0f });
        m_collectAllPuzzlesBtn->SetSizeDelta({ 0.0f, 0.0f });
        m_collectAllPuzzlesBtn->SetBackgroundColor({ 0.3f, 0.3f, 0.3f, 1.0f });
        m_collectAllPuzzlesBtn->SetBackgroundSprite(Engine::AssetManager::GetTexture("Assets/UI/UI Kit/Panel/panel-027.png"), true);
        if (m_collectAllPuzzlesBtn->GetBackground()) m_collectAllPuzzlesBtn->GetBackground()->SetPixelsPerUnitMultiplier(2.0f);
        m_collectAllPuzzlesBtn->SetColors(btnColors);
        m_collectAllPuzzlesBtn->SetText("Collect All Puzzles");
        if (m_collectAllPuzzlesBtn->GetLabel())
        {
            m_collectAllPuzzlesBtn->GetLabel()->SetFontSize(1.1f);
            m_collectAllPuzzlesBtn->GetLabel()->SetColor({ 0.9f, 0.9f, 0.9f, 1.0f });
        }
        m_collectAllPuzzlesBtn->SetRenderOrder(2);

        // ---- Cheat button callbacks ----
        m_infiniteSpeedBtn->onClick = [this]() {
            Game::Player::cheatInfiniteSpeed = !Game::Player::cheatInfiniteSpeed;
            bool isON = Game::Player::cheatInfiniteSpeed;
            m_infiniteSpeedBtn->SetText(isON ? "Infinite Speed: ON" : "Infinite Speed: OFF");
            m_infiniteSpeedBtn->SetBackgroundColor(isON ? Engine::Color{ 0.1f, 0.5f, 0.1f, 1.0f }
                                                        : Engine::Color{ 0.3f, 0.3f, 0.3f, 1.0f });
            UI::ColorBlock c;
            if (isON) { c.normalColor = { 0.2f, 0.6f, 0.2f, 1.0f }; c.highlightedColor = { 0.3f, 0.7f, 0.3f, 1.0f }; c.pressedColor = { 0.4f, 0.8f, 0.4f, 1.0f }; }
            else      { c.normalColor = { 0.3f, 0.3f, 0.3f, 1.0f }; c.highlightedColor = { 0.4f, 0.4f, 0.4f, 1.0f }; c.pressedColor = { 0.5f, 0.5f, 0.5f, 1.0f }; }
            m_infiniteSpeedBtn->SetColors(c);
        };

        m_catchImmuneBtn->onClick = [this]() {
            Game::Player::cheatCatchImmune = !Game::Player::cheatCatchImmune;
            bool isON = Game::Player::cheatCatchImmune;
            m_catchImmuneBtn->SetText(isON ? "Catch Immune: ON" : "Catch Immune: OFF");
            m_catchImmuneBtn->SetBackgroundColor(isON ? Engine::Color{ 0.1f, 0.5f, 0.1f, 1.0f }
                                                      : Engine::Color{ 0.3f, 0.3f, 0.3f, 1.0f });
            UI::ColorBlock c;
            if (isON) { c.normalColor = { 0.2f, 0.6f, 0.2f, 1.0f }; c.highlightedColor = { 0.3f, 0.7f, 0.3f, 1.0f }; c.pressedColor = { 0.4f, 0.8f, 0.4f, 1.0f }; }
            else      { c.normalColor = { 0.3f, 0.3f, 0.3f, 1.0f }; c.highlightedColor = { 0.4f, 0.4f, 0.4f, 1.0f }; c.pressedColor = { 0.5f, 0.5f, 0.5f, 1.0f }; }
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
            m_showRenderLimit = !m_showRenderLimit;
            
            if (m_showRenderLimit)
            {
                m_renderLimitBtn->SetText("Show Render Limit: ON");
                m_renderLimitBtn->SetBackgroundColor({ 0.1f, 0.5f, 0.1f, 1.0f }); // Greenish tint
                
                UI::ColorBlock activeColors;
                activeColors.normalColor = { 0.2f, 0.6f, 0.2f, 1.0f };
                activeColors.highlightedColor = { 0.3f, 0.7f, 0.3f, 1.0f };
                activeColors.pressedColor = { 0.4f, 0.8f, 0.4f, 1.0f };
                m_renderLimitBtn->SetColors(activeColors);
            }
            else
            {
                if (Engine::Camera::main)
                {
                    Engine::Camera::main->backgroundColor = m_originalCameraColor;
                }
                
                m_renderLimitBtn->SetText("Show Render Limit: OFF");
                m_renderLimitBtn->SetBackgroundColor({ 0.3f, 0.3f, 0.3f, 1.0f }); // Default tint
                
                UI::ColorBlock inactiveColors;
                inactiveColors.normalColor = { 0.3f, 0.3f, 0.3f, 1.0f };
                inactiveColors.highlightedColor = { 0.4f, 0.4f, 0.4f, 1.0f };
                inactiveColors.pressedColor = { 0.5f, 0.5f, 0.5f, 1.0f };
                m_renderLimitBtn->SetColors(inactiveColors);
            }
        };
        
        m_showCollidersBtn->onClick = [this]() {
            Engine::Physics2D::debugDrawEnabled = !Engine::Physics2D::debugDrawEnabled;
            bool isON = Engine::Physics2D::debugDrawEnabled;
            
            m_showCollidersBtn->SetText(isON ? "Show Colliders: ON" : "Show Colliders: OFF");
            m_showCollidersBtn->SetBackgroundColor(isON ? Engine::Color{ 0.1f, 0.5f, 0.1f, 1.0f } : Engine::Color{ 0.3f, 0.3f, 0.3f, 1.0f });
            
            UI::ColorBlock colors;
            if (isON) {
                colors.normalColor = { 0.2f, 0.6f, 0.2f, 1.0f };
                colors.highlightedColor = { 0.3f, 0.7f, 0.3f, 1.0f };
                colors.pressedColor = { 0.4f, 0.8f, 0.4f, 1.0f };
            } else {
                colors.normalColor = { 0.3f, 0.3f, 0.3f, 1.0f };
                colors.highlightedColor = { 0.4f, 0.4f, 0.4f, 1.0f };
                colors.pressedColor = { 0.5f, 0.5f, 0.5f, 1.0f };
            }
            m_showCollidersBtn->SetColors(colors);
        };
    }
}
