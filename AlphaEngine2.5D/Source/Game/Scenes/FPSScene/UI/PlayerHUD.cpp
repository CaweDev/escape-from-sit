/**
 * @file PlayerHUD.cpp
 * @brief Implementation of the PlayerHUD component
 */

#include "PlayerHUD.hpp"
#include "Game/CCTV/CCTVTerminal.hpp"
#include "Game/PathRenderer/PathRenderer.hpp"
#include "Engine/UI/Core/UIManager.hpp"
#include "Engine/UI/Components/Panel.hpp"
#include "Engine/UI/Components/Text.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Core/AssetManager.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include <iostream>
#include <cmath>
#include "Game/Puzzle/PuzzleManager.h"
#include "Engine/Core/Time.hpp"

namespace Game
{
    PlayerHUD::~PlayerHUD()
    {
        // Canvas and its children are managed by UIManager
    }

    void PlayerHUD::Awake()
    {
        // Create a dedicated canvas for the HUD
        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        m_canvas->SetReferenceResolution(1600.f, 900.0f);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);
        m_canvas->SetSortOrder(100);

        // Create minimap in bottom-left corner
        m_minimap = m_canvas->CreateChild<UI::Minimap>();
        m_minimap->name = "Minimap";
        m_minimap->SetAnchors({ 0.0f, 0.0f }, { 0.0f, 0.0f });
        m_minimap->SetPivot({ 0.0f, 0.0f });
        m_minimap->SetAnchoredPosition({ m_minimapMargin, m_minimapMargin });
        m_minimap->SetSizeDelta({ m_minimapWidth, m_minimapHeight });

        // Create full map centered on screen (hidden by default)
        m_map = m_canvas->CreateChild<UI::Map>();
        m_map->name = "FullMap";
        m_map->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
        m_map->SetPivot({ 0.5f, 0.5f });
        m_map->SetAnchoredPosition({ 0.0f, 0.0f });
        m_map->SetSizeDelta({ m_mapWidth, m_mapHeight });
        m_map->visible = false;
        
        // Create heatmap overlay (hidden by default)
        m_heatmapOverlay = m_canvas->CreateChild<UI::HeatmapOverlay>();
        m_heatmapOverlay->name = "HeatmapOverlay";
        m_heatmapOverlay->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
        m_heatmapOverlay->SetPivot({ 0.5f, 0.5f });
        m_heatmapOverlay->SetAnchoredPosition({ 0.0f, 0.0f });
        m_heatmapOverlay->SetSizeDelta({ m_mapWidth, m_mapHeight });
        m_heatmapOverlay->SetHeatGradient(
            Engine::Color(0.0f, 0.0f, 0.8f, 0.3f), // Cold (blue, semi-transparent)
            Engine::Color(1.0f, 1.0f, 0.0f, 0.6f), // Warm (yellow)
            Engine::Color(1.0f, 0.0f, 0.0f, 0.9f)  // Hot (red)
        );
        m_heatmapOverlay->SetOpacity(0.75f);
        m_heatmapOverlay->SetMinThreshold(0.5f);
        m_heatmapOverlay->SetBorderColor(Engine::Color(0.9f, 0.3f, 0.3f, 1.0f)); // Red border
        m_heatmapOverlay->SetBorderThickness(5.0f);
        m_heatmapOverlay->visible = false;


        if (m_tileMap) SetTileMap(m_tileMap);
        if (m_playerTransform) SetPlayerTransform(m_playerTransform);

        for (auto* tf : m_enemyTransforms)
        {
            if (m_minimap) m_minimap->AddEnemyTransform(tf);
        }

        Engine::Sprite* cctvSpr = Engine::AssetManager::GetSprite("Assets/cctv.png");
        if (m_minimap) m_minimap->SetCCTVSprites(cctvSpr);
        if (m_map)     m_map->SetCCTVSprites(cctvSpr);

        for (auto* tf : m_cctvTransforms)
        {
            if (m_minimap) m_minimap->AddCCTVTransform(tf);
            if (m_map)     m_map->AddCCTVTransform(tf);
        }
        m_map->SetRenderOrder(1000000);
        //m_map->wallColor = Engine::Color(0.f, 0.f, 0.f, 1.f); 
        m_map->SetBorderColor(Engine::Color(0.f, 0.f, 0.f, 1.f)); // Set alpha to 0 if using texture
        //m_map->SetTexture(Engine::AssetManager::GetTexture("Assets/border.png"));

        // Recommendation 2: Create Alert Level UI (top-center)
        m_alertPanel = m_canvas->CreateChild<UI::Panel>();
        m_alertPanel->name = "AlertPanel";
        m_alertPanel->SetAnchors({ 0.5f, 1.0f }, { 0.5f, 1.0f });
        m_alertPanel->SetPivot({ 0.5f, 1.0f });
        m_alertPanel->SetAnchoredPosition({ 0.0f, -20.0f });
        m_alertPanel->SetSizeDelta({ 420.0f, 100.0f });
        m_alertPanel->SetBackgroundColor(Engine::Color(0.0f, 0.0f, 0.0f, 0.7f));

        // Alert text (main status)
        m_alertText = m_alertPanel->CreateChild<UI::Text>();
        m_alertText->name = "AlertText";
        m_alertText->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        m_alertText->SetPivot({ 0.5f, 1.0f });
        m_alertText->SetAnchoredPosition({ 0.0f, -5.0f });
        m_alertText->SetSizeDelta({ 0.0f, 25.0f });
        m_alertText->SetText("SECURITY ALERT");
        m_alertText->SetFontSize(1.6f);
        m_alertText->SetColor(Engine::Color(0.3f, 1.0f, 0.3f, 1.0f));
        m_alertText->SetAnchor(UI::TextAnchor::MiddleCenter);

        // Zone/Guard info text (context info)
        m_zoneInfoText = m_alertPanel->CreateChild<UI::Text>();
        m_zoneInfoText->name = "ZoneInfoText";
        m_zoneInfoText->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        m_zoneInfoText->SetPivot({ 0.5f, 1.0f });
        m_zoneInfoText->SetAnchoredPosition({ 0.0f, -32.0f });
        m_zoneInfoText->SetSizeDelta({ 0.0f, 18.0f });
        m_zoneInfoText->SetText("Guards Responding: 1/4");
        m_zoneInfoText->SetFontSize(1.0f);
        m_zoneInfoText->SetColor(Engine::Color(0.9f, 0.9f, 0.9f, 1.0f));
        m_zoneInfoText->SetAnchor(UI::TextAnchor::MiddleCenter);

        // Alert bar background (at bottom of panel, centered)
        auto* barBg = m_alertPanel->CreateChild<UI::Panel>();
        barBg->name = "AlertBarBg";
        barBg->SetAnchors({ 0.5f, 0.0f }, { 0.5f, 0.0f });
        barBg->SetPivot({ 0.5f, 0.0f });
        barBg->SetAnchoredPosition({ 0.0f, 8.0f });
        barBg->SetSizeDelta({ 380.0f, 24.0f });
        barBg->SetBackgroundColor(Engine::Color(0.2f, 0.2f, 0.2f, 1.0f));

        // Alert bar fill — point-anchored at bottom-left of barBg, sized explicitly each frame
        m_alertBar = barBg->CreateChild<UI::Panel>();
        m_alertBar->name = "AlertBarFill";
        m_alertBar->SetAnchors({ 0.0f, 0.0f }, { 0.0f, 0.0f });
        m_alertBar->SetPivot({ 0.0f, 0.0f });
        m_alertBar->SetAnchoredPosition({ 0.0f, 0.0f });
        m_alertBar->SetSizeDelta({ 95.0f, 24.0f }); // 25% of 380 = 95, height matches barBg
        m_alertBar->SetBackgroundColor(Engine::Color(0.3f, 1.0f, 0.3f, 1.0f)); // Green

        // CCTV Warning Panel (screen border flash)
        m_cctvWarningPanel = m_canvas->CreateChild<UI::Panel>();
        m_cctvWarningPanel->name = "CCTVWarning";
        m_cctvWarningPanel->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_cctvWarningPanel->SetPivot({ 0.5f, 0.5f });
        m_cctvWarningPanel->SetAnchoredPosition({ 0.0f, 0.0f });
        m_cctvWarningPanel->SetSizeDelta({ 0.0f, 0.0f });
        m_cctvWarningPanel->SetBackgroundColor(Engine::Color(1.0f, 0.0f, 0.0f, 0.0f)); // Transparent initially
        m_cctvWarningPanel->visible = false;

        // CCTV Warning Text
        m_cctvWarningText = m_cctvWarningPanel->CreateChild<UI::Text>();
        m_cctvWarningText->name = "CCTVWarningText";
        m_cctvWarningText->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
        m_cctvWarningText->SetPivot({ 0.5f, 0.5f });
        m_cctvWarningText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_cctvWarningText->SetSizeDelta({ 500.0f, 80.0f });
        m_cctvWarningText->SetText("CCTV DETECTED!");
        m_cctvWarningText->SetFontSize(3.0f);
        m_cctvWarningText->SetColor(Engine::Color(1.0f, 0.0f, 0.0f, 1.0f));
        m_cctvWarningText->SetAnchor(UI::TextAnchor::MiddleCenter);
        
        // Stamina Bar (bottom-left corner, above minimap)
        m_staminaPanel = m_canvas->CreateChild<UI::Panel>();
        m_staminaPanel->name = "StaminaPanel";
        m_staminaPanel->SetAnchors({ 0.0f, 0.0f }, { 0.0f, 0.0f });
        m_staminaPanel->SetPivot({ 0.0f, 0.0f });
        m_staminaPanel->SetAnchoredPosition({ m_minimapMargin, m_minimapMargin + m_minimapHeight + 10.0f });
        m_staminaPanel->SetSizeDelta({ 200.0f, 24.0f });
        m_staminaPanel->SetBackgroundColor(Engine::Color(0.2f, 0.2f, 0.2f, 0.9f));

        // Stamina fill bar — point-anchored at bottom-left of panel.
        // Width and height are set explicitly via sizeDelta each frame.
        m_staminaBar = m_staminaPanel->CreateChild<UI::Panel>();
        m_staminaBar->name = "StaminaBarFill";
        m_staminaBar->SetAnchors({ 0.0f, 0.0f }, { 0.0f, 0.0f });
        m_staminaBar->SetPivot({ 0.0f, 0.0f });
        m_staminaBar->SetAnchoredPosition({ 0.0f, 0.0f });
        m_staminaBar->SetSizeDelta({ 200.0f, 24.0f });
        m_staminaBar->SetBackgroundColor(Engine::Color(0.3f, 0.8f, 1.0f, 0.9f));

        // Stamina text — overlaid on top of the bar, same bounds as panel
        m_staminaText = m_staminaPanel->CreateChild<UI::Text>();
        m_staminaText->name = "StaminaText";
        m_staminaText->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_staminaText->SetPivot({ 0.5f, 0.5f });
        m_staminaText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_staminaText->SetSizeDelta({ 0.0f, 0.0f });
        m_staminaText->SetText("STAMINA 100%");
        m_staminaText->SetFontSize(0.9f);
        m_staminaText->SetColor(Engine::Color::white());
        m_staminaText->SetAnchor(UI::TextAnchor::MiddleCenter);
        
        // High Score Display (top-right)
        m_scorePanel = m_canvas->CreateChild<UI::Panel>();
        m_scorePanel->name = "ScorePanel";
        m_scorePanel->SetAnchors({ 1.0f, 1.0f }, { 1.0f, 1.0f });
        m_scorePanel->SetPivot({ 1.0f, 1.0f });
        m_scorePanel->SetAnchoredPosition({ -20.0f, -20.0f });
        m_scorePanel->SetSizeDelta({ 200.0f, 80.0f });
        m_scorePanel->SetBackgroundColor(Engine::Color(0.0f, 0.0f, 0.0f, 0.7f));
        
        m_highScoreText = m_scorePanel->CreateChild<UI::Text>();
        m_highScoreText->name = "HighScoreText";
        m_highScoreText->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        m_highScoreText->SetPivot({ 0.5f, 1.0f });
        m_highScoreText->SetAnchoredPosition({ 0.0f, -5.0f });
        m_highScoreText->SetSizeDelta({ 0.0f, 20.0f });
        m_highScoreText->SetText("Best: --:--");
        m_highScoreText->SetFontSize(0.9f);
        m_highScoreText->SetColor(Engine::Color(1.0f, 0.8f, 0.0f, 1.0f)); // Gold
        m_highScoreText->SetAnchor(UI::TextAnchor::MiddleCenter);
        
        m_currentTimeText = m_scorePanel->CreateChild<UI::Text>();
        m_currentTimeText->name = "CurrentTimeText";
        m_currentTimeText->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 0.0f });
        m_currentTimeText->SetPivot({ 0.5f, 0.0f });
        m_currentTimeText->SetAnchoredPosition({ 0.0f, 5.0f });
        m_currentTimeText->SetSizeDelta({ 0.0f, 30.0f });
        m_currentTimeText->SetText("00:00");
        m_currentTimeText->SetFontSize(1.5f);
        m_currentTimeText->SetColor(Engine::Color::white());
        m_currentTimeText->SetAnchor(UI::TextAnchor::MiddleCenter);
        
        // Guard Proximity Warning (center-bottom)
        m_proximityText = m_canvas->CreateChild<UI::Text>();
        m_proximityText->name = "ProximityWarning";
        m_proximityText->SetAnchors({ 0.5f, 0.0f }, { 0.5f, 0.0f });
        m_proximityText->SetPivot({ 0.5f, 0.0f });
        m_proximityText->SetAnchoredPosition({ 0.0f, 100.0f });
        m_proximityText->SetSizeDelta({ 400.0f, 40.0f });
        m_proximityText->SetText("");
        m_proximityText->SetFontSize(1.5f);
        m_proximityText->SetColor(Engine::Color(1.0f, 0.3f, 0.0f, 1.0f)); // Orange
        m_proximityText->SetAnchor(UI::TextAnchor::MiddleCenter);
        m_proximityText->visible = false;
        
        // Hack Status Text (center-top)
        m_hackStatusText = m_canvas->CreateChild<UI::Text>();
        m_hackStatusText->name = "HackStatusText";
        m_hackStatusText->SetAnchors({ 0.5f, 1.0f }, { 0.5f, 1.0f });
        m_hackStatusText->SetPivot({ 0.5f, 1.0f });
        m_hackStatusText->SetAnchoredPosition({ 0.0f, -140.0f });
        m_hackStatusText->SetSizeDelta({ 600.0f, 40.0f });
        m_hackStatusText->SetText("");
        m_hackStatusText->SetFontSize(1.2f);
        m_hackStatusText->SetColor(Engine::Color(0.3f, 1.0f, 0.3f, 1.0f)); // Green
        m_hackStatusText->SetAnchor(UI::TextAnchor::MiddleCenter);
        m_hackStatusText->visible = false;
        
        // Zone Entry Notification (center of screen)
        m_zoneEntryPanel = m_canvas->CreateChild<UI::Panel>();
        m_zoneEntryPanel->name = "ZoneEntryPanel";
        m_zoneEntryPanel->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
        m_zoneEntryPanel->SetPivot({ 0.5f, 0.5f });
        m_zoneEntryPanel->SetAnchoredPosition({ 0.0f, 0.0f }); // Dead center of screen
        m_zoneEntryPanel->SetSizeDelta({ 400.0f, 100.0f });
        m_zoneEntryPanel->SetBackgroundColor(Engine::Color(0.0f, 0.0f, 0.0f, 0.85f));
        m_zoneEntryPanel->visible = false;
        
        m_zoneEntryText = m_zoneEntryPanel->CreateChild<UI::Text>();
        m_zoneEntryText->name = "ZoneEntryText";
        m_zoneEntryText->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_zoneEntryText->SetPivot({ 0.5f, 0.5f });
        m_zoneEntryText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_zoneEntryText->SetSizeDelta({ -10.0f, -10.0f });
        m_zoneEntryText->SetText("ENTERING\nZONE A");
        m_zoneEntryText->SetFontSize(2.0f); // Larger font for center display
        m_zoneEntryText->SetColor(Engine::Color(0.3f, 1.0f, 1.0f, 1.0f)); // Cyan
        m_zoneEntryText->SetAnchor(UI::TextAnchor::MiddleCenter);


        //BookCount text Panel
        m_bookCountPanel = m_canvas->CreateChild<UI::Panel>();
        m_bookCountPanel->name = "BookCountPanel";
        m_bookCountPanel->SetAnchors({ 0.0f, 1.0f }, { 0.0f, 1.0f });
        m_bookCountPanel->SetPivot({ 0.0f, 1.0f });
        m_bookCountPanel->SetAnchoredPosition({ 20.0f, -20.0f });
        m_bookCountPanel->SetSizeDelta({ 420.0f, 40.0f });
        m_bookCountPanel->SetBackgroundColor(Engine::Color(0.0f, 0.0f, 0.0f, 0.7f));

        // BookCount Text
        m_bookCountText = m_bookCountPanel->CreateChild<UI::Text>();
        m_bookCountText->name = "BookCountText";
        m_bookCountText->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        m_bookCountText->SetPivot({ 0.5f, 1.0f });
        m_bookCountText->SetAnchoredPosition({ 0.0f, -5.0f });
        m_bookCountText->SetSizeDelta({ 0.0f, 25.0f });
        m_bookCountText->SetText("Books Collected: 0/6");
        m_bookCountText->SetFontSize(1.6f);
        m_bookCountText->SetColor(Engine::Color(0.3f, 1.0f, 0.3f, 1.0f));
        m_bookCountText->SetAnchor(UI::TextAnchor::MiddleCenter);


        //POP UP TEXT
        // --- POPUP PANEL (Center of Screen) ---
        m_popupPanel = m_canvas->CreateChild<UI::Panel>();
        m_popupPanel->name = "PopupPanel";
        m_popupPanel->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f }); // Anchor to Dead Center
        m_popupPanel->SetPivot({ 0.5f, 0.5f });                   // Pivot in Dead Center
        m_popupPanel->SetAnchoredPosition({ 0.0f, 0.0f });
        m_popupPanel->SetSizeDelta({ 600.0f, 150.0f });           // Make it wide enough for a sentence
        m_popupPanel->SetBackgroundColor(Engine::Color(0.0f, 0.0f, 0.0f, 0.85f)); // Dark background
        m_popupPanel->visible = false;                            // Hide it by default!

        // --- POPUP TEXT ---
        m_popupText = m_popupPanel->CreateChild<UI::Text>();
        m_popupText->name = "PopupText";
        m_popupText->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });  // Stretch text across the whole panel
        m_popupText->SetPivot({ 0.5f, 0.5f });
        m_popupText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_popupText->SetSizeDelta({ 0.0f, 0.0f });
        m_popupText->SetText("");
        m_popupText->SetFontSize(2.0f);                           // Make it nice and big
        m_popupText->SetColor(Engine::Color(1.0f, 1.0f, 1.0f, 1.0f)); // White text
        m_popupText->SetAnchor(UI::TextAnchor::MiddleCenter);
        
        // Access Card Display (top-left, below book count)
        m_cardPanel = m_canvas->CreateChild<UI::Panel>();
        m_cardPanel->name = "CardPanel";
        m_cardPanel->SetAnchors({ 0.0f, 1.0f }, { 0.0f, 1.0f });
        m_cardPanel->SetPivot({ 0.0f, 1.0f });
        m_cardPanel->SetAnchoredPosition({ 20.0f, -70.0f }); // Below book count panel
        m_cardPanel->SetSizeDelta({ 420.0f, 40.0f });
        m_cardPanel->SetBackgroundColor(Engine::Color(0.0f, 0.0f, 0.0f, 0.7f));

        m_cardText = m_cardPanel->CreateChild<UI::Text>();
        m_cardText->name = "CardText";
        m_cardText->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_cardText->SetPivot({ 0.5f, 0.5f });
        m_cardText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_cardText->SetSizeDelta({ 0.0f, 0.0f });
        m_cardText->SetText("Access Cards: [ ] [ ] [ ] [ ] (0/4)");
        m_cardText->SetFontSize(1.2f);
        m_cardText->SetColor(Engine::Color(1.0f, 0.8f, 0.0f, 1.0f)); // Gold color
        m_cardText->SetAnchor(UI::TextAnchor::MiddleCenter);


        // If we have a transform, use it as player transform by default
        if (transform && !m_playerTransform)
        {
            SetPlayerTransform(transform);
        }
        if (m_map && m_zoneGetter) {
            m_map->SetZoneGetter(m_zoneGetter);
        }
    }

    void PlayerHUD::Update()
    {
        // Toggle map with M key
        if (Engine::Input::GetKeyDown(Engine::KeyCode::M))
        {
            ToggleMap();
        }

        // Toggle heatmap with F2 key
        if (Engine::Input::GetKeyDown(Engine::KeyCode::F2))
        {
            ToggleHeatmap();
        }
        
        // Toggle path to terminals with G/P/R/Y keys (for Green/Purple/Red/Yellow cards)
        if (Engine::Input::GetKeyDown(Engine::KeyCode::G) && m_cardsCollected[0]) {
            TogglePath(0); // Green card -> Zone B
        }
        if (Engine::Input::GetKeyDown(Engine::KeyCode::P) && m_cardsCollected[1]) {
            TogglePath(1); // Purple card -> Zone C
        }
        if (Engine::Input::GetKeyDown(Engine::KeyCode::R) && m_cardsCollected[2]) {
            TogglePath(2); // Red card -> Zone D
        }
        if (Engine::Input::GetKeyDown(Engine::KeyCode::Y) && m_cardsCollected[3]) {
            TogglePath(3); // Yellow card -> Zone A
        }
        
        // Update active path from current position
        if (m_pathVisible && m_activePathCardId >= 0) {
            UpdateActivePath();
        }

        // Update CCTV warning timer
        if (m_cctvWarningTimer > 0.0f) {
            m_cctvWarningTimer -= Engine::Time::deltaTime;

            if (m_cctvWarningTimer <= 0.0f && m_cctvWarningPanel) {
                m_cctvWarningPanel->visible = false;
            }
            else if (m_cctvWarningPanel) {
                // Flash effect with lower alpha for better visibility
                float flashSpeed = 8.0f;
                float alpha = 0.5f + 0.5f * std::abs(std::sin(m_cctvWarningTimer * flashSpeed));
                m_cctvWarningPanel->SetBackgroundColor(Engine::Color(1.0f, 0.0f, 0.0f, alpha * 0.3f)); // Reduced from 0.8f to 0.3f
            }
        }

        // Update hack status timer
        if (m_hackStatusTimer > 0.0f) {
            m_hackStatusTimer -= Engine::Time::deltaTime;

            if (m_hackStatusTimer <= 0.0f && m_hackStatusText) {
                m_hackStatusText->visible = false;
            }
        }

        // Update zone entry timer
        if (m_zoneEntryTimer > 0.0f) {
            m_zoneEntryTimer -= Engine::Time::deltaTime;

            if (m_zoneEntryTimer <= 0.0f && m_zoneEntryPanel) {
                m_zoneEntryPanel->visible = false;
            }
        }

        // Update proximity warning visibility
        if (m_proximityText) {
            m_proximityText->visible = (m_proximityIntensity > 0.01f);
        }
        //Book Collected
        if (m_bookCountText != nullptr) {
            std::string countString = "Books Collected: " + std::to_string(PuzzleManager::booksCollected) + " / 6";
            m_bookCountText->SetText(countString);
        }

        if (PuzzleManager::popupTimer > 0.0f) {

            // Ensure the panel is visible and the text is updated
            if (!m_popupPanel->visible) {
                m_popupPanel->visible = true;
                m_popupText->SetText(PuzzleManager::activeBookText);
            }

            // Count down the timer
            PuzzleManager::popupTimer -= Engine::Time::deltaTime;   

            // If the timer just hit zero, hide the panel!
            if (PuzzleManager::popupTimer <= 0.0f) {
                m_popupPanel->visible = false;
            }
        }
    }

    void PlayerHUD::OnDestroy()
    {
        if (m_canvas)
        {
            UI::UIManager::GetInstance()->DestroyCanvas(m_canvas);
            m_canvas = nullptr;
            m_minimap = nullptr;
            m_map = nullptr;
        }
    }

    void PlayerHUD::SetTileMap(Engine::TileMap* tileMap)
    {
        m_tileMap = tileMap;

        if (m_minimap)
        {
            m_minimap->SetTileMap(tileMap);
        }

        if (m_map)
        {
            m_map->SetTileMap(tileMap);
        }
        
        if (m_heatmapOverlay)
        {
            m_heatmapOverlay->SetTileMap(tileMap);
        }
    }

    void PlayerHUD::SetPlayerTransform(Engine::Transform* playerTransform)
    {
        m_playerTransform = playerTransform;

        if (m_minimap)
        {
            m_minimap->SetPlayerTransform(playerTransform);
        }

        if (m_map)
        {
            m_map->SetPlayerTransform(playerTransform);
        }
        
        if (m_heatmapOverlay)
        {
            m_heatmapOverlay->SetPlayerTransform(playerTransform);
        }
    }

    void PlayerHUD::AddEnemyTransform(Engine::Transform* enemyTransform)
    {
        if (!enemyTransform) return;


        m_enemyTransforms.push_back(enemyTransform);


        if (m_minimap)
        {
            m_minimap->AddEnemyTransform(enemyTransform);
        }
        if (m_map) {
            m_map->AddEnemyTransform(enemyTransform);
        }
    }

    void PlayerHUD::AddCCTVTransform(Engine::Transform* cctvTransform)
    {
        if (!cctvTransform) return;
        m_cctvTransforms.push_back(cctvTransform);
        if (m_minimap) m_minimap->AddCCTVTransform(cctvTransform);
        if (m_map)     m_map->AddCCTVTransform(cctvTransform);
    }

    void PlayerHUD::ClearCCTVs()
    {
        m_cctvTransforms.clear();
        if (m_minimap) m_minimap->ClearCCTVs();
        if (m_map)     m_map->ClearCCTVs();
    }

    void PlayerHUD::ToggleMap()
    {
        m_mapVisible = !m_mapVisible;

        if (m_map)
        {
            m_map->visible = m_mapVisible;
        }
    }

    void PlayerHUD::ShowMap()
    {
        m_mapVisible = true;

        if (m_map)
        {
            m_map->visible = true;
        }
    }

    void PlayerHUD::HideMap()
    {
        m_mapVisible = false;

        if (m_map)
        {
            m_map->visible = false;
        }
    }
    
    void PlayerHUD::ToggleHeatmap()
    {
        m_heatmapVisible = !m_heatmapVisible;

        if (m_heatmapOverlay)
        {
            m_heatmapOverlay->visible = m_heatmapVisible;
        }
    }

    void PlayerHUD::ShowHeatmap()
    {
        m_heatmapVisible = true;

        if (m_heatmapOverlay)
        {
            m_heatmapOverlay->visible = true;
        }
    }

    void PlayerHUD::HideHeatmap()
    {
        m_heatmapVisible = false;

        if (m_heatmapOverlay)
        {
            m_heatmapOverlay->visible = false;
        }
    }
    
    void PlayerHUD::SetHeatmap(Heatmap* heatmap)
    {
        if (m_heatmapOverlay)
        {
            m_heatmapOverlay->SetHeatmap(heatmap);
        }
    }

    void PlayerHUD::UpdateAlertLevel(int stage, int priorityZone, int guardCount)
    {
        if (!m_alertText || !m_alertBar || !m_zoneInfoText) return;

        // Convert zone number to zone letter using the centralized function
        char zoneLetter = Game::ZoneIdToLetter(priorityZone);

        // Determine stage info
        const char* stageName = "UNKNOWN";
        Engine::Color stageColor = Engine::Color::white();
        float barFill = 0.0f;

        switch (stage) {
        case 1:
            stageName = "LOW";
            stageColor = Engine::Color(0.3f, 1.0f, 0.3f, 1.0f); // Green
            barFill = 0.25f;
            break;
        case 2:
            stageName = "MEDIUM";
            stageColor = Engine::Color(1.0f, 1.0f, 0.3f, 1.0f); // Yellow
            barFill = 0.5f;
            break;
        case 3:
            stageName = "HIGH";
            stageColor = Engine::Color(1.0f, 0.6f, 0.0f, 1.0f); // Orange
            barFill = 0.75f;
            break;
        case 4:
            stageName = "CRITICAL";
            stageColor = Engine::Color(1.0f, 0.2f, 0.2f, 1.0f); // Red
            barFill = 1.0f;
            break;
        default:
            stageName = "SAFE";
            stageColor = Engine::Color(0.3f, 1.0f, 0.3f, 1.0f);
            barFill = 0.0f;
            break;
        }

        // Option 3: Detailed/Technical view
        std::string mainText = std::string("ALERT: ") + stageName;
        std::string subText = std::string("Guards Focus: Zone ") + zoneLetter + " | Stage " + std::to_string(stage) + "/4";

        // Update UI elements
        m_alertText->SetText(mainText.c_str());
        m_alertText->SetColor(stageColor);
        
        m_zoneInfoText->SetText(subText.c_str());
        
        m_alertBar->SetBackgroundColor(stageColor);
        m_alertBar->SetSizeDelta({ 380.0f * barFill, 24.0f }); // width = fill%, height matches barBg
    }

    void PlayerHUD::SetAlertUIVisible(bool visible)
    {
        if (m_alertPanel) {
            m_alertPanel->visible = visible;
        }
    }

    void PlayerHUD::ShowCCTVWarning(bool show)
    {
        if (!m_cctvWarningPanel) return;

        if (show) {
            m_cctvWarningPanel->visible = true;
            m_cctvWarningTimer = m_cctvWarningDuration;
        }
        else {
            m_cctvWarningPanel->visible = false;
            m_cctvWarningTimer = 0.0f;
        }
    }
    void PlayerHUD::SetZoneGetter(std::function<int(int, int)> fn)
    {
        m_zoneGetter = fn;

        // if map already created, apply immediately
        if (m_map)
            m_map->SetZoneGetter(m_zoneGetter);
    }

    void PlayerHUD::SetAlertDetailMode(bool detailed)
    {
        // This method can be used to toggle between different alert display modes
        // For now, it's a placeholder for future functionality
        // Could toggle between:
        // - Simple: Just alert level bar
        // - Detailed: Show guard counts, zone info, etc.
    }
    
    void PlayerHUD::UpdateStamina(float current, float max)
    {
        if (!m_staminaBar || !m_staminaText) return;
        
        float ratio = (max > 0.0f) ? (current / max) : 0.0f;
        if (ratio < 0.0f) ratio = 0.0f;
        if (ratio > 1.0f) ratio = 1.0f;
        
        // Point anchor at {0,0}: sizeDelta directly sets the absolute width and height.
        const float panelWidth  = 200.0f;
        const float panelHeight = 24.0f;
        m_staminaBar->SetSizeDelta({ panelWidth * ratio, panelHeight });
        
        // Update color based on stamina level
        Engine::Color barColor;
        if (ratio > 0.5f) {
            barColor = Engine::Color(0.3f, 0.8f, 1.0f, 0.9f); // Cyan (good)
        } else if (ratio > 0.2f) {
            barColor = Engine::Color(1.0f, 0.8f, 0.0f, 0.9f); // Yellow (warning)
        } else {
            barColor = Engine::Color(1.0f, 0.3f, 0.0f, 0.9f); // Orange/Red (critical)
        }
        m_staminaBar->SetBackgroundColor(barColor);
        
        // Update text
        int percent = (int)(ratio * 100.0f);
        std::string text = "STAMINA " + std::to_string(percent) + "%";
        m_staminaText->SetText(text.c_str());
    }
    
    void PlayerHUD::UpdateHighScore(float highScore)
    {
        if (!m_highScoreText) return;
        
        int minutes = (int)(highScore / 60.0f);
        int seconds = (int)highScore % 60;
        
        std::string text;
        if (highScore < 999999.0f) {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "Best: %02d:%02d", minutes, seconds);
            text = buffer;
        } else {
            text = "Best: --:--";
        }
        
        m_highScoreText->SetText(text.c_str());
    }
    
    void PlayerHUD::UpdateCurrentTime(float currentTime)
    {
        if (!m_currentTimeText) return;
        
        int minutes = (int)(currentTime / 60.0f);
        int seconds = (int)currentTime % 60;
        
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
        m_currentTimeText->SetText(buffer);
    }
    
    void PlayerHUD::UpdateGuardProximity(float nearestDistance)
    {
        if (!m_proximityText) return;
        
        // Show warning when guard is within 8 units
        const float warningDistance = 8.0f;
        const float criticalDistance = 4.0f;
        
        if (nearestDistance < warningDistance) {
            m_proximityIntensity = 1.0f - (nearestDistance / warningDistance);
            
            std::string warningText;
            Engine::Color warningColor;
            
            if (nearestDistance < criticalDistance) {
                warningText = "!!! GUARD VERY CLOSE !!!";
                warningColor = Engine::Color(1.0f, 0.0f, 0.0f, 1.0f); // Red
            } else {
                warningText = "! Guard Nearby !";
                warningColor = Engine::Color(1.0f, 0.5f, 0.0f, 1.0f); // Orange
            }
            
            m_proximityText->SetText(warningText.c_str());
            m_proximityText->SetColor(warningColor);
            
            // Pulse effect
            float pulse = 0.7f + 0.3f * std::abs(std::sin(Engine::Time::time * 5.0f * m_proximityIntensity));
            Engine::Color pulseColor = warningColor;
            pulseColor.a = pulse;
            m_proximityText->SetColor(pulseColor);
        } else {
            m_proximityIntensity = 0.0f;
        }
    }
    
    void PlayerHUD::ShowHackStatus(const std::string& message, float duration)
    {
        if (!m_hackStatusText) return;
        
        m_hackStatusText->SetText(message.c_str());
        m_hackStatusText->visible = true;
        m_hackStatusTimer = duration;
    }
    
    void PlayerHUD::CheckZoneChange(int zoneId)
    {
        // Initialize on first call (don't show notification on spawn)
        if (m_currentZone == -1) {
            m_currentZone = zoneId;
            std::cout << "[PlayerHUD] Initial zone set to " << Game::ZoneIdToLetter(zoneId) << " (no notification)\n";
            return;
        }
        
        // Check if zone changed
        if (zoneId != m_currentZone && zoneId >= 1 && zoneId <= 4) {
            m_currentZone = zoneId;
            ShowZoneEntry(zoneId);
        }
    }
    
    void PlayerHUD::ShowZoneEntry(int zoneId)
    {
        if (!m_zoneEntryPanel || !m_zoneEntryText) return;
        
        // Convert zone ID to letter (matching the system-wide mapping)
        // Zone 1 = A (Bottom-Left), 2 = B (Top-Left), 3 = C (Top-Right), 4 = D (Bottom-Right)
        char zoneLetter = Game::ZoneIdToLetter(zoneId);
        
        std::string message = std::string("ENTERING\nZONE ") + zoneLetter;
        m_zoneEntryText->SetText(message.c_str());
        
        // Show panel with slide-in animation (could be enhanced with smooth animation)
        m_zoneEntryPanel->visible = true;
        m_zoneEntryTimer = m_zoneEntryDuration;
        
        std::cout << "[PlayerHUD] Showing zone entry notification: Zone " << zoneLetter << "\n";
    }
    
    void PlayerHUD::CollectAccessCard(int cardId, const Engine::Vector2& collectionPos)
    {
        if (cardId < 0 || cardId >= 4) return;
        if (m_cardsCollected[cardId]) return; // Already collected
        
        m_cardsCollected[cardId] = true;
        m_totalCardsCollected++;
        
        UpdateCardDisplay();
        
        // Show notification
        const char* cardNames[] = {"Green", "Purple", "Red", "Yellow"};
        std::string message = std::string("Collected ") + cardNames[cardId] + " Access Card (" + 
                              std::to_string(m_totalCardsCollected) + "/4)";
        ShowHackStatus(message, 3.0f);
        
        // Determine which zone this card belongs to (cardId matches zone index)
        // Card 0 (Green) = Zone 2 (B), Card 1 (Purple) = Zone 3 (C), 
        // Card 2 (Red) = Zone 4 (D), Card 3 (Yellow) = Zone 1 (A)
        int targetZoneId = -1;
        switch (cardId) {
            case 0: targetZoneId = 2; break; // Green -> Zone B
            case 1: targetZoneId = 3; break; // Purple -> Zone C
            case 2: targetZoneId = 4; break; // Red -> Zone D
            case 3: targetZoneId = 1; break; // Yellow -> Zone A
        }
        
        // Don't auto-activate path - let player toggle it with key press
        if (targetZoneId >= 1 && targetZoneId <= 4) {
            char zoneLetter = Game::ZoneIdToLetter(targetZoneId);
            const char* keyName = "?";
            switch(cardId) {
                case 0: keyName = "G"; break; // Green
                case 1: keyName = "P"; break; // Purple
                case 2: keyName = "R"; break; // Red
                case 3: keyName = "Y"; break; // Yellow
            }
            
            std::string pathMessage = std::string("Press '") + keyName + "' to toggle path to Zone " + zoneLetter + " Terminal";
            ShowHackStatus(pathMessage, 5.0f);
            
            std::cout << "[PlayerHUD] Card collected. Press '" << keyName 
                      << "' to toggle path to Zone " << zoneLetter << " terminal\n";
        }
        
        if (m_totalCardsCollected >= 4) {
            ShowHackStatus("All access cards collected! Control room doors unlocked!", 5.0f);
        }
    }
    
    bool PlayerHUD::HasAccessCard(int cardId) const
    {
        if (cardId < 0 || cardId >= 4) return false;
        return m_cardsCollected[cardId];
    }
    
    void PlayerHUD::UpdateCardDisplay()
    {
        if (!m_cardText) return;
        
        // Build display string showing collected cards
        std::string cardStatus = "Access Cards: ";
        const char* cardSymbols[] = {"G", "P", "R", "Y"}; // Green, Purple, Red, Yellow
        
        for (int i = 0; i < 4; ++i) {
            if (m_cardsCollected[i]) {
                cardStatus += "[" + std::string(cardSymbols[i]) + "] ";
            } else {
                cardStatus += "[ ] ";
            }
        }
        
        cardStatus += "(" + std::to_string(m_totalCardsCollected) + "/4)";
        m_cardText->SetText(cardStatus.c_str());
    }
    
    void PlayerHUD::SetTerminalPosition(int zoneId, const Engine::Vector2& position)
    {
        if (zoneId < 1 || zoneId > 4) return;
        m_terminalPositions[zoneId - 1] = position;
        std::cout << "[PlayerHUD] Set Zone " << Game::ZoneIdToLetter(zoneId) 
                  << " terminal position to (" << position.x << ", " << position.y << ")\n";
    }
    
    Engine::Vector2 PlayerHUD::GetTerminalPosition(int zoneId) const
    {
        if (zoneId < 1 || zoneId > 4) return Engine::Vector2(0, 0);
        return m_terminalPositions[zoneId - 1];
    }
    
    void PlayerHUD::TogglePath(int cardId)
    {
        if (cardId < 0 || cardId >= 4) return;
        if (!m_cardsCollected[cardId]) return;
        
        // Get path renderer reference early for clearing operations
        auto player = Engine::GameObject::Find("Player");
        Game::PathRenderer* pathRenderer = nullptr;
        if (player) {
            pathRenderer = player->GetComponent<Game::PathRenderer>();
        }
        
        // Toggle visibility
        if (m_activePathCardId == cardId && m_pathVisible) {
            // Same card, turn off
            m_pathVisible = false;
            m_activePathCardId = -1;
            
            // Clear path from renderer
            if (pathRenderer) {
                pathRenderer->ClearPath();
            }
            
            ShowHackStatus("Path hidden", 2.0f);
        } else {
            // Different card or was off, turn on
            // IMPORTANT: Clear old path before setting new one
            if (pathRenderer) {
                pathRenderer->ClearPath();
            }
            
            m_pathVisible = true;
            m_activePathCardId = cardId;
            
            // Update path immediately
            UpdateActivePath();
            
            // Determine which zone
            int targetZoneId = -1;
            switch (cardId) {
                case 0: targetZoneId = 2; break; // Green -> Zone B
                case 1: targetZoneId = 3; break; // Purple -> Zone C
                case 2: targetZoneId = 4; break; // Red -> Zone D
                case 3: targetZoneId = 1; break; // Yellow -> Zone A
            }
            
            if (targetZoneId >= 1 && targetZoneId <= 4) {
                char zoneLetter = Game::ZoneIdToLetter(targetZoneId);
                std::string message = std::string("Path to Zone ") + zoneLetter + " Terminal shown";
                ShowHackStatus(message, 2.0f);
            }
        }
    }
    
    void PlayerHUD::UpdateActivePath()
    {
        if (!m_pathVisible || m_activePathCardId < 0 || m_activePathCardId >= 4) {
            // Clear path from minimap
            if (m_minimap) {
                m_minimap->ClearPath();
            }
            return;
        }
        
        if (!m_playerTransform || !m_tileMap) return;
        
        // Get player's current position
        Engine::Vector2 currentPos(m_playerTransform->position.x, m_playerTransform->position.y);
        
        // Determine which zone to path to
        int targetZoneId = -1;
        switch (m_activePathCardId) {
            case 0: targetZoneId = 2; break; // Green -> Zone B
            case 1: targetZoneId = 3; break; // Purple -> Zone C
            case 2: targetZoneId = 4; break; // Red -> Zone D
            case 3: targetZoneId = 1; break; // Yellow -> Zone A
        }
        
        if (targetZoneId < 1 || targetZoneId > 4) return;
        
        Engine::Vector2 terminalPos = GetTerminalPosition(targetZoneId);
        
        // Check if terminal position is valid (not at origin)
        if (terminalPos.x == 0.0f && terminalPos.y == 0.0f) {
            std::cout << "[PlayerHUD] ERROR: No terminal position set for Zone " << Game::ZoneIdToLetter(targetZoneId) 
                      << "! Please add 'T' marker in that zone using Level Editor.\n";
            m_pathVisible = false;
            m_activePathCardId = -1;
            ShowHackStatus("ERROR: No terminal in that zone!", 3.0f);
            return;
        }
        
        // Get or create path renderer to compute the path
        auto player = Engine::GameObject::Find("Player");
        if (!player) return;
        
        auto pathRenderer = player->GetComponent<Game::PathRenderer>();
        if (!pathRenderer) {
            pathRenderer = player->AddComponent<Game::PathRenderer>();
            pathRenderer->tileMap = m_tileMap;
            pathRenderer->playerTransform = m_playerTransform;
        }
        
        // Compute path using PathRenderer's A* (but don't render in 3D)
        pathRenderer->SetPath(currentPos, terminalPos);
        
        // Get the waypoints and display them on the minimap instead!
        if (m_minimap) {
            m_minimap->SetPathWaypoints(pathRenderer->pathWaypoints);
        }
    }

} // namespace Game