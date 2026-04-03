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
#include "Engine/UI/Components/Image.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Core/AssetManager.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <cmath>
#include "Game/Puzzle/PuzzleManager.hpp"
#include "Engine/Audio/SoundManager.hpp"
#include "Game/Player/PlayerPowerUpController.hpp"
#include "Game/Puzzle/PuzzleManager.hpp"

namespace Game
{
    static bool TryReadVec2(const Json::Value& node, const char* key, AEVec2& out)
    {
        if (!node.isMember(key) || !node[key].isObject()) return false;
        const auto& v = node[key];
        out.x = v.get("x", out.x).asFloat();
        out.y = v.get("y", out.y).asFloat();
        return true;
    }

    static void ApplyLayoutNode(UI::UIElement* e, const Json::Value& layout)
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

    static void ApplyLayoutRecursive(UI::UIElement* e, const Json::Value& layout)
    {
        if (!e) return;
        ApplyLayoutNode(e, layout);
        for (auto* c : e->GetChildren()) ApplyLayoutRecursive(c, layout);
    }

    static void ApplyLayoutToCanvas(UI::Canvas* canvas, const Json::Value& layout)
    {
        if (!canvas || !layout.isObject()) return;
        for (auto* root : canvas->GetRootElements()) ApplyLayoutRecursive(root, layout);
    }

    struct PlayerHUDConfig : public Engine::IConfig
    {
        Json::Value layout;
        float uiToggleVolume = 0.75f;
        float audioDebugClickVolume = 1.0f;
        float nearestBookDebugDistance = 8.0f;
        float bookNearDistance = 2.0f;
        float bookFarDistance = 6.0f;
        float bookLoopVolume = 1.0f;

        float cctvWarningFlashSpeed = 8.0f;
        float cctvWarningPanelAlphaScale = 0.3f;

        float bullyPulseSpeed = 6.0f;
        float annoyingPulseSpeed = 4.0f;

        float guardWarningDistance = 8.0f;
        float guardCriticalDistance = 4.0f;
        float guardPulseSpeedBase = 5.0f;

        float quizResultPopupDuration = 0.8f;

        void FromJson(const Json::Value& j) override
        {
            layout = j["layout"];
            uiToggleVolume = j.get("uiToggleVolume", uiToggleVolume).asFloat();
            audioDebugClickVolume = j.get("audioDebugClickVolume", audioDebugClickVolume).asFloat();
            nearestBookDebugDistance = j.get("nearestBookDebugDistance", nearestBookDebugDistance).asFloat();
            bookNearDistance = j.get("bookNearDistance", bookNearDistance).asFloat();
            bookFarDistance = j.get("bookFarDistance", bookFarDistance).asFloat();
            bookLoopVolume = j.get("bookLoopVolume", bookLoopVolume).asFloat();

            cctvWarningFlashSpeed = j.get("cctvWarningFlashSpeed", cctvWarningFlashSpeed).asFloat();
            cctvWarningPanelAlphaScale = j.get("cctvWarningPanelAlphaScale", cctvWarningPanelAlphaScale).asFloat();

            bullyPulseSpeed = j.get("bullyPulseSpeed", bullyPulseSpeed).asFloat();
            annoyingPulseSpeed = j.get("annoyingPulseSpeed", annoyingPulseSpeed).asFloat();

            guardWarningDistance = j.get("guardWarningDistance", guardWarningDistance).asFloat();
            guardCriticalDistance = j.get("guardCriticalDistance", guardCriticalDistance).asFloat();
            guardPulseSpeedBase = j.get("guardPulseSpeedBase", guardPulseSpeedBase).asFloat();

            quizResultPopupDuration = j.get("quizResultPopupDuration", quizResultPopupDuration).asFloat();
        }
    };

    static PlayerHUDConfig g_playerHUDConfig;
    static bool g_playerHUDConfigLoaded = false;

    static const PlayerHUDConfig& GetPlayerHUDConfig()
    {
        if (!g_playerHUDConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/player_hud.json", g_playerHUDConfig);
            g_playerHUDConfigLoaded = true;
        }
        return g_playerHUDConfig;
    }

    // HUD panel texture paths and background colors.
    // These are static so they can later be updated from a config/read-from-file flow.
    static std::string s_alertPanelTexturePath     = "Assets/UI/UI Kit/Panel/panel-009.png";
    static Engine::Color s_alertPanelColor     = Engine::Color(0.05f, 0.05f, 0.05f, 0.88f);
    static std::string s_minimapPanelTexturePath   = "Assets/UI/UI Kit/Panel/panel-009.png";
    static Engine::Color s_minimapPanelColor   = Engine::Color(0.05f, 0.05f, 0.05f, 0.92f);
    static std::string s_alertBarBgTexturePath     = "Assets/UI/UI Kit/Panel/panel-009.png";
    static Engine::Color s_alertBarBgColor     = Engine::Color(0.05f, 0.05f, 0.05f, 0.92f);
    static std::string s_staminaPanelTexturePath   = "Assets/UI/UI Kit/Panel/panel-009.png";
    static Engine::Color s_staminaPanelColor   = Engine::Color(0.05f, 0.05f, 0.05f, 0.92f);
    static std::string s_scorePanelTexturePath     = "Assets/UI/UI Kit/Panel/panel-009.png";
    static Engine::Color s_scorePanelColor     = Engine::Color(0.05f, 0.05f, 0.05f, 0.88f);
    static std::string s_zoneEntryPanelTexturePath = "Assets/UI/UI Kit/Panel/panel-009.png";
    static Engine::Color s_zoneEntryPanelColor = Engine::Color(0.05f, 0.05f, 0.05f, 0.92f);
    static std::string s_bookCountPanelTexturePath = "Assets/UI/UI Kit/Panel/panel-009.png";
    static Engine::Color s_bookCountPanelColor = Engine::Color(0.05f, 0.05f, 0.05f, 0.88f);
    static std::string s_bookPopupPanelTexturePath = "Assets/UI/UI Kit/Panel/panel-009.png";
    static Engine::Color s_bookPopupPanelColor = Engine::Color(0.05f, 0.05f, 0.05f, 0.92f);
    static std::string s_cardPanelTexturePath      = "Assets/UI/UI Kit/Panel/panel-009.png";
    static Engine::Color s_cardPanelColor      = Engine::Color(0.05f, 0.05f, 0.05f, 0.88f);
    static std::string s_powerUpPanelTexturePath   = "Assets/UI/UI Kit/Panel/panel-009.png";

    PlayerHUD::~PlayerHUD()
    {
        // Canvas and its children are managed by UIManager
    }

    void PlayerHUD::UpdateNPCApproach(int annoyingCount, float nearestAnnoyingDist, int bullyCount, float nearestBullyDist)
    {
        // Bully: stronger warning
        if (bullyCount > 0 && m_bullyApproachText) {
            char buf[128];
            if (nearestBullyDist < 99999.0f) {
                snprintf(buf, sizeof(buf), "BULLY APPROACHING! %d near, closest: %.1fm", bullyCount, nearestBullyDist);
            } else {
                snprintf(buf, sizeof(buf), "BULLY APPROACHING! %d near", bullyCount);
            }
            m_bullyApproachText->SetText(buf);
            m_bullyApproachText->visible = true;
            m_bullyApproachTimer = m_approachDisplayDuration;
            // No audio for NPC approach alerts (handled elsewhere or intentionally silent)
        }

        // Annoying classmates: softer notification
        if (annoyingCount > 0 && m_annoyingApproachText) {
            char buf[128];
            if (nearestAnnoyingDist < 99999.0f) {
                snprintf(buf, sizeof(buf), "Annoying classmate nearby: %d, closest: %.1fm", annoyingCount, nearestAnnoyingDist);
            } else {
                snprintf(buf, sizeof(buf), "Annoying classmate nearby: %d", annoyingCount);
            }
            m_annoyingApproachText->SetText(buf);
            m_annoyingApproachText->visible = true;
            m_annoyingApproachTimer = m_approachDisplayDuration;
            // Ensure starting alpha is fully visible so pulsing is noticeable
            Engine::Color col = m_annoyingApproachText->GetColor();
            col.a = 1.0f;
            m_annoyingApproachText->SetColor(col);
        }
    }

    void PlayerHUD::Awake()
    {
        // Create a dedicated canvas for the HUD
        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        m_canvas->SetReferenceResolution(1600.f, 900.0f);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);
        m_canvas->SetSortOrder(100);

        // Create minimap in bottom-left corner
        Engine::Texture2D* minimapPanelTexture = Engine::AssetManager::GetTexture(s_minimapPanelTexturePath.c_str());
        auto* minimapPanel = m_canvas->CreateChild<UI::Panel>();
        minimapPanel->name = "MinimapPanel";
        minimapPanel->SetAnchors({ 0.0f, 0.0f }, { 0.0f, 0.0f });
        minimapPanel->SetPivot({ 0.0f, 0.0f });
        minimapPanel->SetAnchoredPosition({ m_minimapMargin - 4.0f, m_minimapMargin - 4.0f });
        minimapPanel->SetSizeDelta({ m_minimapWidth + 8.0f, m_minimapHeight + 8.0f });
        minimapPanel->SetBackgroundColor(s_minimapPanelColor);
        if (minimapPanelTexture)
        {
            minimapPanel->SetBackgroundSprite(minimapPanelTexture);
            if (auto* bg = minimapPanel->GetBackground())
            {
                bg->SetImageType(UI::ImageType::Sliced);
                bg->SetPixelsPerUnitMultiplier(1.0f);
            }
        }

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

        Engine::Sprite* cctvSpr = Engine::AssetManager::GetSprite("Assets/Sprites/cctv.png");
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
        Engine::Texture2D* alertPanelTexture = Engine::AssetManager::GetTexture(s_alertPanelTexturePath.c_str());
        m_alertPanel = m_canvas->CreateChild<UI::Panel>();
        m_alertPanel->name = "AlertPanel";
        m_alertPanel->SetAnchors({ 0.5f, 1.0f }, { 0.5f, 1.0f });
        m_alertPanel->SetPivot({ 0.5f, 1.0f });
        m_alertPanel->SetAnchoredPosition({ 0.0f, -20.0f });
        m_alertPanel->SetSizeDelta({ 420.0f, 100.0f });
        m_alertPanel->SetBackgroundColor(s_alertPanelColor);
        // Alert panel texture (editable via s_alertPanelTexturePath)
        if (alertPanelTexture)
        {
            m_alertPanel->SetBackgroundSprite(alertPanelTexture);
            if (auto* bg = m_alertPanel->GetBackground())
            {
                bg->SetImageType(UI::ImageType::Sliced);
                bg->SetPixelsPerUnitMultiplier(1.0f);
            }
        }

        // Alert text (main status)
        m_alertText = m_alertPanel->CreateChild<UI::Text>();
        m_alertText->name = "AlertText";
        m_alertText->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        m_alertText->SetPivot({ 0.5f, 1.0f });
        m_alertText->SetAnchoredPosition({ 0.0f, -5.0f });
        m_alertText->SetSizeDelta({ -20.0f, 25.0f });
        m_alertText->SetText("SECURITY ALERT");
        m_alertText->SetFontSize(1.35f);
        m_alertText->SetColor(Engine::Color(0.3f, 1.0f, 0.3f, 1.0f));
        m_alertText->SetAnchor(UI::TextAnchor::MiddleCenter);

        // Zone/Guard info text (context info)
        m_zoneInfoText = m_alertPanel->CreateChild<UI::Text>();
        m_zoneInfoText->name = "ZoneInfoText";
        m_zoneInfoText->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        m_zoneInfoText->SetPivot({ 0.5f, 1.0f });
        m_zoneInfoText->SetAnchoredPosition({ 0.0f, -32.0f });
        m_zoneInfoText->SetSizeDelta({ -20.0f, 18.0f });
        m_zoneInfoText->SetText("Guards Responding: 1/4");
        m_zoneInfoText->SetFontSize(0.9f);
        m_zoneInfoText->SetColor(Engine::Color(0.9f, 0.9f, 0.9f, 1.0f));
        m_zoneInfoText->SetAnchor(UI::TextAnchor::MiddleCenter);

        // Alert bar background (at bottom of panel, centered)
        Engine::Texture2D* alertBarBgTexture = Engine::AssetManager::GetTexture(s_alertBarBgTexturePath.c_str());
        auto* barBg = m_alertPanel->CreateChild<UI::Panel>();
        barBg->name = "AlertBarBg";
        barBg->SetAnchors({ 0.5f, 0.0f }, { 0.5f, 0.0f });
        barBg->SetPivot({ 0.5f, 0.0f });
        barBg->SetAnchoredPosition({ 0.0f, 8.0f });
        barBg->SetSizeDelta({ 380.0f, 24.0f });
        barBg->SetBackgroundColor(s_alertBarBgColor);
        // Alert bar background texture (editable via s_alertBarBgTexturePath)
        if (alertBarBgTexture)
        {
            barBg->SetBackgroundSprite(alertBarBgTexture);
            if (auto* bg = barBg->GetBackground())
            {
                bg->SetImageType(UI::ImageType::Sliced);
                bg->SetPixelsPerUnitMultiplier(1.0f);
            }
        }

        // Alert bar fill � point-anchored at bottom-left of barBg, sized explicitly each frame
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
        
        // Stamina Bar (aligned to minimap and placed above it)
        Engine::Texture2D* staminaPanelTexture = Engine::AssetManager::GetTexture(s_staminaPanelTexturePath.c_str());
        if (m_minimap)
        {
            // Parent to minimap so horizontal alignment always matches minimap exactly.
            m_staminaPanel = m_minimap->CreateChild<UI::Panel>();
            m_staminaPanel->SetAnchors({ 0.0f, 1.0f }, { 0.0f, 1.0f });
            m_staminaPanel->SetPivot({ 0.0f, 0.0f });
            m_staminaPanel->SetAnchoredPosition({ 0.0f, 10.0f });
        }
        else
        {
            // Fallback (should rarely happen): keep old canvas-relative placement.
            m_staminaPanel = m_canvas->CreateChild<UI::Panel>();
            m_staminaPanel->SetAnchors({ 0.0f, 0.0f }, { 0.0f, 0.0f });
            m_staminaPanel->SetPivot({ 0.0f, 0.0f });
            m_staminaPanel->SetAnchoredPosition({ m_minimapMargin, m_minimapMargin + m_minimapHeight + 10.0f });
        }
        m_staminaPanel->name = "StaminaPanel";
        m_staminaPanel->SetSizeDelta({ m_minimapWidth, 24.0f });
        m_staminaPanel->SetBackgroundColor(s_staminaPanelColor);
        // Stamina panel texture (editable via s_staminaPanelTexturePath)
        if (staminaPanelTexture)
        {
            m_staminaPanel->SetBackgroundSprite(staminaPanelTexture);
            if (auto* bg = m_staminaPanel->GetBackground())
            {
                bg->SetImageType(UI::ImageType::Sliced);
                bg->SetPixelsPerUnitMultiplier(1.0f);
            }
        }

        // Stamina fill bar � point-anchored at bottom-left of panel.
        // Width and height are set explicitly via sizeDelta each frame.
        m_staminaBar = m_staminaPanel->CreateChild<UI::Panel>();
        m_staminaBar->name = "StaminaBarFill";
        m_staminaBar->SetAnchors({ 0.0f, 0.0f }, { 0.0f, 0.0f });
        m_staminaBar->SetPivot({ 0.0f, 0.0f });
        m_staminaBar->SetAnchoredPosition({ 4.0f, 4.0f });
        m_staminaBar->SetSizeDelta({ m_minimapWidth - 8.0f, 16.0f });
        m_staminaBar->SetBackgroundColor(Engine::Color(0.3f, 0.8f, 1.0f, 0.9f));

        // Stamina text � overlaid on top of the bar, same bounds as panel
        m_staminaText = m_staminaPanel->CreateChild<UI::Text>();
        m_staminaText->name = "StaminaText";
        m_staminaText->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_staminaText->SetPivot({ 0.5f, 0.5f });
        m_staminaText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_staminaText->SetSizeDelta({ -8.0f, -4.0f });
        m_staminaText->SetText("STAMINA 100%");
        m_staminaText->SetFontSize(0.78f);
        m_staminaText->SetColor(Engine::Color::white());
        m_staminaText->SetAnchor(UI::TextAnchor::MiddleCenter);
        
        // High Score Display (top-right)
        Engine::Texture2D* scorePanelTexture = Engine::AssetManager::GetTexture(s_scorePanelTexturePath.c_str());
        m_scorePanel = m_canvas->CreateChild<UI::Panel>();
        m_scorePanel->name = "ScorePanel";
        m_scorePanel->SetAnchors({ 1.0f, 1.0f }, { 1.0f, 1.0f });
        m_scorePanel->SetPivot({ 1.0f, 1.0f });
        m_scorePanel->SetAnchoredPosition({ -20.0f, -20.0f });
        m_scorePanel->SetSizeDelta({ 200.0f, 80.0f });
        m_scorePanel->SetBackgroundColor(s_scorePanelColor);
        // Score panel texture (editable via s_scorePanelTexturePath)
        if (scorePanelTexture)
        {
            m_scorePanel->SetBackgroundSprite(scorePanelTexture);
            if (auto* bg = m_scorePanel->GetBackground())
            {
                bg->SetImageType(UI::ImageType::Sliced);
                bg->SetPixelsPerUnitMultiplier(1.0f);
            }
        }
        
        m_highScoreText = m_scorePanel->CreateChild<UI::Text>();
        m_highScoreText->name = "HighScoreText";
        m_highScoreText->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        m_highScoreText->SetPivot({ 0.5f, 1.0f });
        m_highScoreText->SetAnchoredPosition({ 0.0f, -20.0f });
        m_highScoreText->SetSizeDelta({ -16.0f, 20.0f });
        m_highScoreText->SetText("Best: --:--");
        m_highScoreText->SetFontSize(1.4f);
        m_highScoreText->SetColor(Engine::Color(1.0f, 0.8f, 0.0f, 1.0f)); // Gold
        m_highScoreText->SetAnchor(UI::TextAnchor::MiddleCenter);
        
        m_currentTimeText = m_scorePanel->CreateChild<UI::Text>();
        m_currentTimeText->name = "CurrentTimeText";
        m_currentTimeText->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 0.0f });
        m_currentTimeText->SetPivot({ 0.5f, 0.0f });
        m_currentTimeText->SetAnchoredPosition({ 0.0f, 6.0f });
        m_currentTimeText->SetSizeDelta({ -16.0f, 30.0f });
        m_currentTimeText->SetText("00:00");
        m_currentTimeText->SetFontSize(1.25f);
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
        
        // NPC Approach indicators (center-bottom, above proximity)
        m_bullyApproachText = m_canvas->CreateChild<UI::Text>();
        m_bullyApproachText->name = "BullyApproach";
        m_bullyApproachText->SetAnchors({ 0.5f, 0.0f }, { 0.5f, 0.0f });
        m_bullyApproachText->SetPivot({ 0.5f, 0.0f });
        m_bullyApproachText->SetAnchoredPosition({ 0.0f, 140.0f });
        m_bullyApproachText->SetSizeDelta({ 600.0f, 30.0f });
        m_bullyApproachText->SetText("");
        m_bullyApproachText->SetFontSize(1.2f);
        m_bullyApproachText->SetColor(Engine::Color(1.0f, 0.2f, 0.2f, 1.0f)); // Red
        m_bullyApproachText->SetAnchor(UI::TextAnchor::MiddleCenter);
        m_bullyApproachText->visible = false;

        m_annoyingApproachText = m_canvas->CreateChild<UI::Text>();
        m_annoyingApproachText->name = "AnnoyingApproach";
        m_annoyingApproachText->SetAnchors({ 0.5f, 0.0f }, { 0.5f, 0.0f });
        m_annoyingApproachText->SetPivot({ 0.5f, 0.0f });
        m_annoyingApproachText->SetAnchoredPosition({ 0.0f, 120.0f });
        m_annoyingApproachText->SetSizeDelta({ 600.0f, 26.0f });
        m_annoyingApproachText->SetText("");
        m_annoyingApproachText->SetFontSize(1.05f);
        m_annoyingApproachText->SetColor(Engine::Color(0.6f, 1.0f, 0.6f, 1.0f)); // Greenish
        m_annoyingApproachText->SetAnchor(UI::TextAnchor::MiddleCenter);
        m_annoyingApproachText->visible = false;
        
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
        Engine::Texture2D* zoneEntryPanelTexture = Engine::AssetManager::GetTexture(s_zoneEntryPanelTexturePath.c_str());
        m_zoneEntryPanel = m_canvas->CreateChild<UI::Panel>();
        m_zoneEntryPanel->name = "ZoneEntryPanel";
        m_zoneEntryPanel->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
        m_zoneEntryPanel->SetPivot({ 0.5f, 0.5f });
        m_zoneEntryPanel->SetAnchoredPosition({ 0.0f, 0.0f }); // Dead center of screen
        m_zoneEntryPanel->SetSizeDelta({ 400.0f, 100.0f });
        m_zoneEntryPanel->SetBackgroundColor(s_zoneEntryPanelColor);
        // Zone entry panel texture (editable via s_zoneEntryPanelTexturePath)
        if (zoneEntryPanelTexture)
        {
            m_zoneEntryPanel->SetBackgroundSprite(zoneEntryPanelTexture);
            if (auto* bg = m_zoneEntryPanel->GetBackground())
            {
                bg->SetImageType(UI::ImageType::Sliced);
                bg->SetPixelsPerUnitMultiplier(1.0f);
            }
        }
        m_zoneEntryPanel->visible = false;
        
        m_zoneEntryText = m_zoneEntryPanel->CreateChild<UI::Text>();
        m_zoneEntryText->name = "ZoneEntryText";
        m_zoneEntryText->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_zoneEntryText->SetPivot({ 0.5f, 0.5f });
        m_zoneEntryText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_zoneEntryText->SetSizeDelta({ -10.0f, -10.0f });
        m_zoneEntryText->SetText("ENTERING ZONE A");
        m_zoneEntryText->SetFontSize(2.0f); // Larger font for center display
        m_zoneEntryText->SetColor(Engine::Color(0.3f, 1.0f, 1.0f, 1.0f)); // Cyan
        m_zoneEntryText->SetAnchor(UI::TextAnchor::MiddleCenter);


        //BookCount text Panel
        Engine::Texture2D* bookCountPanelTexture = Engine::AssetManager::GetTexture(s_bookCountPanelTexturePath.c_str());
        m_bookCountPanel = m_canvas->CreateChild<UI::Panel>();
        m_bookCountPanel->name = "BookCountPanel";
        m_bookCountPanel->SetAnchors({ 0.0f, 1.0f }, { 0.0f, 1.0f });
        m_bookCountPanel->SetPivot({ 0.0f, 1.0f });
        m_bookCountPanel->SetAnchoredPosition({ 20.0f, -20.0f });
        m_bookCountPanel->SetSizeDelta({ 420.0f, 40.0f });
        m_bookCountPanel->SetBackgroundColor(s_bookCountPanelColor);
        // Book count panel texture (editable via s_bookCountPanelTexturePath)
        if (bookCountPanelTexture)
        {
            m_bookCountPanel->SetBackgroundSprite(bookCountPanelTexture);
            if (auto* bg = m_bookCountPanel->GetBackground())
            {
                bg->SetImageType(UI::ImageType::Sliced);
                bg->SetPixelsPerUnitMultiplier(1.0f);
            }
        }

        // BookCount Text
        m_bookCountText = m_bookCountPanel->CreateChild<UI::Text>();
        m_bookCountText->name = "BookCountText";
        m_bookCountText->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        m_bookCountText->SetPivot({ 0.5f, 1.0f });
        m_bookCountText->SetAnchoredPosition({ 0.0f, -5.0f });
        m_bookCountText->SetSizeDelta({ -20.0f, 25.0f });
        m_bookCountText->SetText("Books Collected: 0/6");
        m_bookCountText->SetFontSize(1.35f);
        m_bookCountText->SetColor(Engine::Color(0.3f, 1.0f, 0.3f, 1.0f));
        m_bookCountText->SetAnchor(UI::TextAnchor::MiddleCenter);

        // --- BOOK POPUP PANEL (Background) ---
        Engine::Texture2D* bookPopupPanelTexture = Engine::AssetManager::GetTexture(s_bookPopupPanelTexturePath.c_str());
        m_bookPopupPanel = m_canvas->CreateChild<UI::Panel>();
        m_bookPopupPanel->name = "BookPopupPanel";
        m_bookPopupPanel->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f }); // Dead Center
        m_bookPopupPanel->SetPivot({ 0.5f, 0.5f });
        m_bookPopupPanel->SetAnchoredPosition({ 0.0f, 0.0f });
        m_bookPopupPanel->SetSizeDelta({ 800.0f, 450.0f });
        m_bookPopupPanel->SetBackgroundColor(s_bookPopupPanelColor);
        // Book popup panel texture (editable via s_bookPopupPanelTexturePath)
        if (bookPopupPanelTexture)
        {
            m_bookPopupPanel->SetBackgroundSprite(bookPopupPanelTexture);
            if (auto* bg = m_bookPopupPanel->GetBackground())
            {
                bg->SetImageType(UI::ImageType::Sliced);
                bg->SetPixelsPerUnitMultiplier(1.0f);
            }
        }
        m_bookPopupPanel->visible = false; // Hidden by default

        // --- BOOK TITLE TEXT (Top) ---
        m_bookTitleText = m_bookPopupPanel->CreateChild<UI::Text>();
        m_bookTitleText->name = "BookTitleText";
        m_bookTitleText->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f }); // Pin to top
        m_bookTitleText->SetPivot({ 0.5f, 1.0f });
        m_bookTitleText->SetAnchoredPosition({ 0.0f, -30.0f }); // Push down slightly
        m_bookTitleText->SetSizeDelta({ 0.0f, 50.0f });
        m_bookTitleText->SetFontSize(2.5f);
        m_bookTitleText->SetColor(Engine::Color(1.0f, 0.8f, 0.2f, 1.0f)); // Gold text
        m_bookTitleText->SetAnchor(UI::TextAnchor::UpperCenter);

        // --- BOOK LORE TEXT (Bottom) ---
        m_bookLoreText = m_bookPopupPanel->CreateChild<UI::Text>();
        m_bookLoreText->name = "BookLoreText";
        m_bookLoreText->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        m_bookLoreText->SetPivot({ 0.5f, 1.0f });
        m_bookLoreText->SetAnchoredPosition({ 0.0f, -100.0f });
        m_bookLoreText->SetSizeDelta({ -40.0f, 60.0f });
        m_bookLoreText->SetFontSize(1.5f);
        m_bookLoreText->SetColor(Engine::Color(1.0f, 1.0f, 1.0f, 1.0f));
        m_bookLoreText->SetAnchor(UI::TextAnchor::UpperCenter);

        // --- NEW: OPTION 1 TEXT ---
        m_option1Text = m_bookPopupPanel->CreateChild<UI::Text>(); // <--- THE MISSING CREATION LINE!
        m_option1Text->name = "Option1Text";
        m_option1Text->SetAnchors({ 0.0f, 1.0f }, { 0.0f, 1.0f });
        m_option1Text->SetPivot({ 0.0f, 1.0f });
        m_option1Text->SetAnchoredPosition({ 60.0f, -200.0f });
        m_option1Text->SetSizeDelta({ -200.0f, 40.0f });
        m_option1Text->SetFontSize(1.3f);
        m_option1Text->SetColor(Engine::Color(0.8f, 0.8f, 0.8f, 1.0f));
        m_option1Text->SetAnchor(UI::TextAnchor::MiddleLeft);

        // --- NEW: OPTION 2 TEXT ---
        m_option2Text = m_bookPopupPanel->CreateChild<UI::Text>(); // <--- THE MISSING CREATION LINE!
        m_option2Text->name = "Option2Text";
        m_option2Text->SetAnchors({ 0.0f, 1.0f }, { 0.0f, 1.0f });
        m_option2Text->SetPivot({ 0.0f, 1.0f });
        m_option2Text->SetAnchoredPosition({ 60.0f, -250.0f });
        m_option2Text->SetSizeDelta({ -200.0f, 40.0f });
        m_option2Text->SetFontSize(1.3f);
        m_option2Text->SetColor(Engine::Color(0.8f, 0.8f, 0.8f, 1.0f));
        m_option2Text->SetAnchor(UI::TextAnchor::MiddleLeft);

        // --- NEW: OPTION 3 TEXT ---
        m_option3Text = m_bookPopupPanel->CreateChild<UI::Text>(); 
        m_option3Text->name = "Option3Text";
        m_option3Text->SetAnchors({ 0.0f, 1.0f }, { 0.0f, 1.0f });
        m_option3Text->SetPivot({ 0.0f, 1.0f });
        m_option3Text->SetAnchoredPosition({ 60.0f, -300.0f });
        m_option3Text->SetSizeDelta({ -200.0f, 40.0f });
        m_option3Text->SetFontSize(1.3f);
        m_option3Text->SetColor(Engine::Color(0.8f, 0.8f, 0.8f, 1.0f));
        m_option3Text->SetAnchor(UI::TextAnchor::MiddleLeft);

        m_bookGradeText = m_bookPopupPanel->CreateChild<UI::Text>();
        m_bookGradeText->name = "BookGradeText";
        m_bookGradeText->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f }); // Dead Center
        m_bookGradeText->SetPivot({ 0.5f, 0.5f });
        m_bookGradeText->SetAnchoredPosition({ 0.0f, -15.0f });
        m_bookGradeText->SetSizeDelta({ 100.0f, 100.0f });
        m_bookGradeText->SetFontSize(20.0f); 
        m_bookGradeText->SetAnchor(UI::TextAnchor::MiddleCenter);
        m_bookGradeText->visible = false;

        
        // Access Card Display (top-left, below book count)
        Engine::Texture2D* cardPanelTexture = Engine::AssetManager::GetTexture(s_cardPanelTexturePath.c_str());
        m_cardPanel = m_canvas->CreateChild<UI::Panel>();
        m_cardPanel->name = "CardPanel";
        m_cardPanel->SetAnchors({ 0.0f, 1.0f }, { 0.0f, 1.0f });
        m_cardPanel->SetPivot({ 0.0f, 1.0f });
        m_cardPanel->SetAnchoredPosition({ 20.0f, -70.0f }); // Below book count panel
        m_cardPanel->SetSizeDelta({ 420.0f, 40.0f });
        m_cardPanel->SetBackgroundColor(s_cardPanelColor);
        // Access card panel texture (editable via s_cardPanelTexturePath)
        if (cardPanelTexture)
        {
            m_cardPanel->SetBackgroundSprite(cardPanelTexture);
            if (auto* bg = m_cardPanel->GetBackground())
            {
                bg->SetImageType(UI::ImageType::Sliced);
                bg->SetPixelsPerUnitMultiplier(1.0f);
            }
        }

        m_cardText = m_cardPanel->CreateChild<UI::Text>();
        m_cardText->name = "CardText";
        m_cardText->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_cardText->SetPivot({ 0.5f, 0.5f });
        m_cardText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_cardText->SetSizeDelta({ -20.0f, -4.0f });
        m_cardText->SetText("Access Cards: [ ] [ ] [ ] [ ] (0/4)");
        m_cardText->SetFontSize(1.05f);
        m_cardText->SetColor(Engine::Color(1.0f, 0.8f, 0.0f, 1.0f)); // Gold color
        m_cardText->SetAnchor(UI::TextAnchor::MiddleCenter);

        // Power Up timer UI
        m_powerUpTextures[0] = Engine::AssetManager::GetTexture("Assets/Sprites/Speed.png"); // Speed
        m_powerUpTextures[1] = Engine::AssetManager::GetTexture("Assets/Sprites/InfStamina.png"); // Infinite stamina
        m_powerUpTextures[2] = Engine::AssetManager::GetTexture("Assets/Sprites/Shield.png"); // Shield
        m_powerUpTextures[3] = Engine::AssetManager::GetTexture("Assets/Sprites/Invis.png"); // Invisibility

        Engine::Texture2D* powerUpPanelTexture = Engine::AssetManager::GetTexture(s_powerUpPanelTexturePath.c_str());
        constexpr float powerUpPanelScale = 1.15f; // +15% panel size

        const float panelWidth = (240.0f * powerUpPanelScale) + 100.0f; 
        const float panelHeight = (56.0f * powerUpPanelScale) + 25.0f;  // +50 total on Y axis

        const int slotCount = 4;
        const float slotWidth = 72.0f;
        const float slotHeight = 70.0f;
        const float sidePadding = 10.0f;
        const float yPos = 6.0f;
        const float spacing = (panelWidth - (2.0f * sidePadding) - slotWidth) / (slotCount - 1);

        m_powerUpPanel = m_canvas->CreateChild<UI::Panel>();
        m_powerUpPanel->name = "PowerUpPanel";
        m_powerUpPanel->SetAnchors({ 1.0f, 1.0f }, { 1.0f, 1.0f });
        m_powerUpPanel->SetPivot({ 1.0f, 1.0f });
        m_powerUpPanel->SetAnchoredPosition({ -20.0f, -110.0f });
        m_powerUpPanel->SetSizeDelta({ panelWidth, panelHeight });
        m_powerUpPanel->SetBackgroundColor(Engine::Color(0.05f, 0.05f, 0.05f, 0.65f));
        if (powerUpPanelTexture)
        {
            m_powerUpPanel->SetBackgroundSprite(powerUpPanelTexture);
            if (auto* bg = m_powerUpPanel->GetBackground())
            {
                bg->SetImageType(UI::ImageType::Sliced);
                bg->SetPixelsPerUnitMultiplier(1.0f);
            }
        }

        for (int i = 0; i < slotCount; ++i)
        {
            auto* slot = m_powerUpPanel->CreateChild<UI::Panel>();
            slot->SetAnchors({ 0.0f, 0.0f }, { 0.0f, 0.0f });
            slot->SetPivot({ 0.0f, 0.0f });
            slot->SetAnchoredPosition({ sidePadding + spacing * i, yPos });
            slot->SetSizeDelta({ slotWidth, slotHeight });
            slot->SetBackgroundColor(Engine::Color(0.12f, 0.12f, 0.12f, 0.9f));
            if (powerUpPanelTexture)
            {
                slot->SetBackgroundSprite(powerUpPanelTexture);
                if (auto* bg = slot->GetBackground())
                {
                    bg->SetImageType(UI::ImageType::Sliced);
                    bg->SetPixelsPerUnitMultiplier(1.0f);
                }
            }

            m_powerUpIcons[i] = slot->CreateChild<UI::Image>();
            m_powerUpIcons[i]->SetAnchors({ 0.0f, 0.0f }, { 0.0f, 0.0f });
            m_powerUpIcons[i]->SetPivot({ 0.0f, 0.0f });

            const float iconWidth = 50.0f;
            const float iconHeight = 50.0f;
            const float iconX = (slotWidth - iconWidth) * 0.5f;
            const float iconY = (slotHeight - iconHeight) * 0.75f;

            m_powerUpIcons[i]->SetAnchoredPosition({ iconX, iconY });
            m_powerUpIcons[i]->SetSizeDelta({ iconWidth, iconHeight });
            m_powerUpIcons[i]->SetTexture(m_powerUpTextures[i]);
            m_powerUpIcons[i]->SetColor(Engine::Color(1, 1, 1, 0.25f)); // inactive dim

            m_powerUpTimers[i] = slot->CreateChild<UI::Text>();
            m_powerUpTimers[i]->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 0.0f });
            m_powerUpTimers[i]->SetPivot({ 0.5f, 0.0f });
            m_powerUpTimers[i]->SetAnchoredPosition({ 0.0f, 8.0f }); // moved up from 0.0f
            m_powerUpTimers[i]->SetSizeDelta({ 0.0f, 12.0f });
            m_powerUpTimers[i]->SetText("--");
            m_powerUpTimers[i]->SetFontSize(0.75f);
            m_powerUpTimers[i]->SetColor(Engine::Color::white());
            m_powerUpTimers[i]->SetAnchor(UI::TextAnchor::MiddleCenter);
        }

        // If we have a transform, use it as player transform by default
        if (transform && !m_playerTransform)
        {
            SetPlayerTransform(transform);
        }
        if (m_map && m_zoneGetter) {
            m_map->SetZoneGetter(m_zoneGetter);
        }
        
        ApplyLayoutToCanvas(m_canvas, GetPlayerHUDConfig().layout);
    }

    void PlayerHUD::Update()    
    {
        const auto& cfg = GetPlayerHUDConfig();


        if (Engine::Input::GetKeyDown(Engine::KeyCode::O)) {
            Engine::SoundManager::PlaySFX("ui_click", cfg.audioDebugClickVolume);
        }
        // ==========================================
        // BOOK PROXIMITY AUDIO (DEBUG VERSION)
        // ==========================================

        // 1. Failsafe: If HUD lost the player, try to find it again!
        Engine::Transform* activePlayerTrans = m_playerTransform;
        if (!activePlayerTrans) {
            auto pObj = Engine::GameObject::Find("Player");
            if (pObj) activePlayerTrans = pObj->transform;
        }

        if (activePlayerTrans) {
            float closestBookDist = 999.0f;

            for (const auto& book : Game::PuzzleManager::allBooks) {
                if (!book.bookCollected) {
                    // Let's use pure 2D distance just in case the Z-axis is messing us up!
                    float dx = activePlayerTrans->position.x - book.bookPosition.x;
                    float dy = activePlayerTrans->position.y - book.bookPosition.y;
                    float dist = std::sqrt(dx * dx + dy * dy);

                    if (dist < closestBookDist) {
                        closestBookDist = dist;
                    }
                }
            }

            Engine::SoundManager::UpdateDistanceLoop(
                "puzzle_nearby",
                closestBookDist,
                cfg.bookNearDistance,
                cfg.bookFarDistance,
                cfg.bookLoopVolume
            );
        }
        // ==========================================


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
                float flashSpeed = cfg.cctvWarningFlashSpeed;
                float alpha = 0.5f + 0.5f * std::abs(std::sin(m_cctvWarningTimer * flashSpeed));
                m_cctvWarningPanel->SetBackgroundColor(Engine::Color(1.0f, 0.0f, 0.0f, alpha * cfg.cctvWarningPanelAlphaScale));
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
        // Update NPC approach timers
        if (m_bullyApproachTimer > 0.0f) {
            m_bullyApproachTimer -= Engine::Time::deltaTime;
            if (m_bullyApproachTimer <= 0.0f && m_bullyApproachText) {
                m_bullyApproachText->visible = false;
                // Reset SFX flag when approach message ends so it can play on next approach
             
            } else if (m_bullyApproachText) {
                // subtle pulsing alpha for urgency
                float pulse = 0.6f + 0.4f * std::abs(std::sin(Engine::Time::time * cfg.bullyPulseSpeed));
                Engine::Color c = m_bullyApproachText->GetColor();
                c.a = pulse;
                m_bullyApproachText->SetColor(c);
            }
        }

        if (m_annoyingApproachTimer > 0.0f) {
            m_annoyingApproachTimer -= Engine::Time::deltaTime;
            if (m_annoyingApproachTimer <= 0.0f && m_annoyingApproachText) {
                m_annoyingApproachText->visible = false;
            } else if (m_annoyingApproachText) {
                // modest pulse
                float pulse = 0.7f + 0.3f * std::abs(std::sin(Engine::Time::time * cfg.annoyingPulseSpeed));
                Engine::Color c = m_annoyingApproachText->GetColor();
                c.a = pulse;
                m_annoyingApproachText->SetColor(c);
            }
        }
        //Book Collected
        if (m_bookCountText != nullptr) {
            int collected = Game::PuzzleManager::booksCollected;
            int totalBooks = (int)Game::PuzzleManager::allBooks.size();
            if (totalBooks <= 0) totalBooks = 0; // safety
            std::string bs = std::string("Books Collected: ") + std::to_string(collected) + " / " + std::to_string(totalBooks);
            m_bookCountText->SetText(bs.c_str());
        }

        // ==========================================
        // NEW: QUIZ INPUT LOGIC
        // ==========================================
        if (Game::PuzzleManager::isQuizActive) {

            // Force the panel to stay open and show the question
            m_bookPopupPanel->visible = true;
            m_bookTitleText->visible = true;
            m_bookLoreText->visible = true;
            m_option1Text->visible = true;
            m_option2Text->visible = true;
            m_option3Text->visible = true;

            m_bookTitleText->SetColor(Engine::Color(1.0f, 0.8f, 0.2f, 1.0f));

            m_bookTitleText->SetText(Game::PuzzleManager::activeBookTitle);
            m_bookLoreText->SetText(Game::PuzzleManager::activeBookText);
            m_option1Text->SetText(Game::PuzzleManager::activeOpt1);
            m_option2Text->SetText(Game::PuzzleManager::activeOpt2);
            m_option3Text->SetText(Game::PuzzleManager::activeOpt3);

            // Check which key the player presses
            int playerChoice = 0;
            if (Engine::Input::GetKeyDown(Engine::KeyCode::Alpha1)) playerChoice = 1;
            if (Engine::Input::GetKeyDown(Engine::KeyCode::Alpha2)) playerChoice = 2;
            if (Engine::Input::GetKeyDown(Engine::KeyCode::Alpha3)) playerChoice = 3;

            if (playerChoice != 0) { // The player pressed a number!
                if (playerChoice == Game::PuzzleManager::currentCorrectAnswer) {



                    // --- CORRECT ANSWER ---
                    Engine::SoundManager::PlaySFX("card_collect");
                    Game::PuzzleManager::booksCollected++;
                    Game::PuzzleManager::allBooks[Game::PuzzleManager::currentBookId].bookCollected = true;

                    // Destroy the physical book
                    if (Game::PuzzleManager::currentBookObj) {
                        Engine::GameObject::Destroy(Game::PuzzleManager::currentBookObj);
                        Game::PuzzleManager::currentBookObj = nullptr;
                    }

                    // Check if it was the last book
                    if (Game::PuzzleManager::booksCollected >= (int)Game::PuzzleManager::allBooks.size()) {
                        Engine::SoundManager::PlaySFX("all_books_collected");
                    }

                    // Show success text and close after 2 seconds
                    Game::PuzzleManager::activeBookTitle = "CORRECT!";
                    Game::PuzzleManager::activeBookText = "+1 Knowledge Gained.";
                    m_bookTitleText->SetText(Game::PuzzleManager::activeBookTitle);
                    m_bookLoreText->SetText(Game::PuzzleManager::activeBookText);
                    m_bookTitleText->SetColor(Engine::Color(0.2f, 1.0f, 0.2f, 1.0f)); // Flash Green!
                    Game::PuzzleManager::popupTimer = cfg.quizResultPopupDuration;
                    Game::PuzzleManager::isQuizActive = false;

                }
                else {

                    // --- WRONG ANSWER ---
                    Engine::SoundManager::PlaySFX("terminal_hack"); // Reusing your "Access Denied" sound!
                    Game::PuzzleManager::totalWrongAnswers++;
                    // Reset the interactable so they can try again
                    if (Game::PuzzleManager::currentBookObj) {
                        auto pObj = Game::PuzzleManager::currentBookObj->GetComponent<PuzzleObject>();
                        if (pObj) pObj->isReading = false;
                    }

                    // Show failure text and close after 2 seconds
                    Game::PuzzleManager::activeBookTitle = "WRONG!";
                    Game::PuzzleManager::activeBookText = "Try again...";
                    m_bookTitleText->SetText(Game::PuzzleManager::activeBookTitle);
                    m_bookLoreText->SetText(Game::PuzzleManager::activeBookText);
                    m_bookTitleText->SetColor(Engine::Color(1.0f, 0.2f, 0.2f, 1.0f)); // Flash Red!
                    Game::PuzzleManager::popupTimer = cfg.quizResultPopupDuration;
                    Game::PuzzleManager::isQuizActive = false;
                }
            }
        }
        // ==========================================


        if (Game::PuzzleManager::popupTimer > 0.0f) {

            // Only set everything to visible on the exact frame the timer starts
            if (!m_bookPopupPanel->visible) {
                m_bookPopupPanel->visible = true;
                m_bookTitleText->visible = true;
                m_bookLoreText->visible = true;

                m_bookTitleText->SetText(Game::PuzzleManager::activeBookTitle);
                m_bookLoreText->SetText(Game::PuzzleManager::activeBookText);

                // --- THIS IS WHERE THE GRADE ACTUALLY PRINTS ---
                if (!Game::PuzzleManager::activeGrade.empty()) {
                    m_bookGradeText->visible = true;
                    m_bookGradeText->SetText(Game::PuzzleManager::activeGrade);

                    // Dynamic Colors with low Alpha (0.3f) so it acts like a watermark!
                    if (Game::PuzzleManager::activeGrade == "S") {
                        m_bookGradeText->SetColor(Engine::Color(0.2f, 1.0f, 0.2f, 0.3f)); // Neon Green
                    }
                    else if (Game::PuzzleManager::activeGrade == "F") {
                        m_bookGradeText->SetColor(Engine::Color(1.0f, 0.2f, 0.2f, 0.3f)); // Blood Red
                    }
                    else {
                        m_bookGradeText->SetColor(Engine::Color(1.0f, 0.8f, 0.2f, 0.3f)); // Gold/Yellow
                    }
                }
                else {
                    // Normal books don't have grades, so hide the massive letter
                    m_bookGradeText->visible = false;
                }
            }

            // Tick down the timer
            Game::PuzzleManager::popupTimer -= Engine::Time::deltaTime;

            // When the timer finishes, hide everything
            if (Game::PuzzleManager::popupTimer <= 0.0f) {
                m_bookPopupPanel->visible = false;
                m_bookTitleText->visible = false;
                m_bookLoreText->visible = false;

                // Hide and clear the grade stamp when the timer ends
                if (m_bookGradeText) m_bookGradeText->visible = false;
                Game::PuzzleManager::activeGrade = "";
            }
        }

        UpdatePowerUpUI();
    }

    void PlayerHUD::UpdatePowerUpUI()
    {
        if (!gameObject) return;

        auto* controller = gameObject->GetComponent<Game::PlayerPowerUpController>();
        if (!controller) return;

        const Game::PowerUpType types[4] = {
            Game::PowerUpType::SpeedBoost,
            Game::PowerUpType::InfiniteStamina,
            Game::PowerUpType::Shield,
            Game::PowerUpType::Invisibility
        };

        for (int i = 0; i < 4; ++i)
        {
            if (!m_powerUpIcons[i] || !m_powerUpTimers[i]) continue;

            auto it = controller->activeTimers.find(types[i]);
            if (it != controller->activeTimers.end() && it->second > 0.0f)
            {
                const int secs = (int)std::ceil(it->second);
                m_powerUpIcons[i]->SetColor(Engine::Color(1, 1, 1, 1));
                // avoid passing a temporary c_str() directly to SetText to prevent
                // any compiler warnings about temporaries: store in a local string first
                std::string secsTxt = std::to_string(secs) + "s";
                m_powerUpTimers[i]->SetText(secsTxt.c_str());
            }
            else
            {
                m_powerUpIcons[i]->SetColor(Engine::Color(1, 1, 1, 0.25f));
                m_powerUpTimers[i]->SetText("--");
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
			m_powerUpPanel = nullptr;
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

    void PlayerHUD::AddDoor(Engine::Door *door)
    {
        if (!door)
            return;
        m_doors.push_back(door);
        if (m_minimap)
            m_minimap->AddDoor(door);
    }

    void PlayerHUD::ToggleMap()
    {
        m_mapVisible = !m_mapVisible;

        if (m_map)
        {
            m_map->visible = m_mapVisible;
            Engine::SoundManager::PlaySFX("ui_toggle", GetPlayerHUDConfig().uiToggleVolume);
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
            Engine::SoundManager::PlaySFX("ui_toggle", GetPlayerHUDConfig().uiToggleVolume);
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

    void PlayerHUD::UpdateAlertLevel(int stage, int priorityZone, [[maybe_unused]] int guardCount)
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
            Engine::SoundManager::PlaySFX("cctv_alert", 0.8f);

            if (m_banditVisualizer && m_playerTransform) {
                float px = m_playerTransform->position.x;
                float py = m_playerTransform->position.y;
                int zone = m_currentZone;
                m_banditVisualizer->SetCCTVStatus(zone, px, py, true);
            }
        }
        else {
            m_cctvWarningPanel->visible = false;
            m_cctvWarningTimer = 0.0f;

            if (m_banditVisualizer && m_playerTransform) {
                float px = m_playerTransform->position.x;
                float py = m_playerTransform->position.y;
                int zone = m_currentZone;
                m_banditVisualizer->SetCCTVStatus(zone, px, py, false);
            }
        }
    }

    void PlayerHUD::SetZoneGetter(std::function<int(int, int)> fn)
    {
        m_zoneGetter = fn;

        // if map already created, apply immediately
        if (m_map)
            m_map->SetZoneGetter(m_zoneGetter);
    }

    void PlayerHUD::SetAlertDetailMode([[maybe_unused]] bool detailed)
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
        
        // Keep stamina fill inset so it stays inside panel borders.
        const float barWidth  = m_minimapWidth - 8.0f;
        const float barHeight = 16.0f;
        m_staminaBar->SetSizeDelta({ barWidth * ratio, barHeight });
        
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
        const float warningDistance = GetPlayerHUDConfig().guardWarningDistance;
        const float criticalDistance = GetPlayerHUDConfig().guardCriticalDistance;
        
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
            float pulse = 0.7f + 0.3f * std::abs(std::sin(Engine::Time::time * GetPlayerHUDConfig().guardPulseSpeedBase * m_proximityIntensity));
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
        if (zoneId < 1 || zoneId > 4) return;

        if (m_currentZone == -1) {
            m_currentZone = zoneId;

            if (m_banditVisualizer) {
                m_banditVisualizer->SetSelectedZone(zoneId);
            }

            return;
        }

        if (zoneId != m_currentZone) {
            m_currentZone = zoneId;

            if (m_banditVisualizer) {
                m_banditVisualizer->SetSelectedZone(zoneId);
            }

            ShowZoneEntry(zoneId);
        }
    }
    
    void PlayerHUD::ShowZoneEntry(int zoneId)
    {
        if (!m_zoneEntryPanel || !m_zoneEntryText) return;
        
        // Convert zone ID to letter (matching the system-wide mapping)
        // Zone 1 = A (Bottom-Left), 2 = B (Top-Left), 3 = C (Top-Right), 4 = D (Bottom-Right)
        char zoneLetter = Game::ZoneIdToLetter(zoneId);
        
        std::string message = std::string("ENTERING ZONE ") + zoneLetter;
        m_zoneEntryText->SetText(message.c_str());
        
        // Show panel with slide-in animation (could be enhanced with smooth animation)
        m_zoneEntryPanel->visible = true;
        m_zoneEntryTimer = m_zoneEntryDuration;
        Engine::SoundManager::PlaySFX("zone_enter", 0.6f);
    }
    
    void PlayerHUD::CollectAccessCard(int cardId, [[maybe_unused]] const Engine::Vector2& collectionPos)
    {
        if (cardId < 0 || cardId >= 4) return;
        if (m_cardsCollected[cardId]) return; // Already collected
        
        m_cardsCollected[cardId] = true;
        m_totalCardsCollected++;
        Engine::SoundManager::PlaySFX("card_collect");
        
        UpdateCardDisplay();

        // Show notification
        const char* cardNames[] = {"Green", "Purple", "Red", "Yellow"};
        std::string message = std::string("Collected ") + cardNames[cardId] + " Access Card (" + 
                              std::to_string(m_totalCardsCollected) + "/4)";
        ShowHackStatus(message, 3.0f);
        
        // Determine which zone this card unlocks (path leads TO the terminal)
        // Green card (G key) -> Zone B (Green zone) terminal
        // Purple card (P key) -> Zone C (Magenta zone) terminal
        // Red card (R key) -> Zone D (Yellow zone) terminal
        // Yellow card (Y key) -> Zone A (Blue zone) terminal
        int targetZoneId = -1;
        switch (cardId) {
            case 0: targetZoneId = 2; break; // Green card -> Zone B terminal
            case 1: targetZoneId = 3; break; // Purple card -> Zone C terminal
            case 2: targetZoneId = 4; break; // Red card -> Zone D terminal
            case 3: targetZoneId = 1; break; // Yellow card -> Zone A terminal
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
    }
    
    Engine::Vector2 PlayerHUD::GetTerminalPosition(int zoneId) const
    {
        if (zoneId < 1 || zoneId > 4) return Engine::Vector2(0, 0);
        return m_terminalPositions[zoneId - 1];
    }
    
    void PlayerHUD::SetControlRoomPosition(int zoneId, const Engine::Vector2& position)
    {
        if (zoneId < 1 || zoneId > 4) return;
        m_controlRoomPositions[zoneId - 1] = position;
    }
    
    Engine::Vector2 PlayerHUD::GetControlRoomPosition(int zoneId) const
    {
        if (zoneId < 1 || zoneId > 4) return Engine::Vector2(0, 0);
        return m_controlRoomPositions[zoneId - 1];
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
            
            // Clear path from renderer AND minimap
            if (pathRenderer) {
                pathRenderer->ClearPath();
            }
            if (m_minimap) {
                m_minimap->ClearPath(); // CRITICAL: Clear from minimap too!
            }
            
            ShowHackStatus("Path hidden", 2.0f);
        } else {
            // Different card or was off, turn on
            // IMPORTANT: Clear old path before setting new one
            if (pathRenderer) {
                pathRenderer->ClearPath();
            }
            if (m_minimap) {
                m_minimap->ClearPath(); // CRITICAL: Clear old minimap path!
            }
            
            m_pathVisible = true;
            m_activePathCardId = cardId;
            
            // Update path immediately
            UpdateActivePath();
            
            // Determine which zone terminal to route to
            // Green (G) -> Zone B, Purple (P) -> Zone C, Red (R) -> Zone D, Yellow (Y) -> Zone A
            int targetZoneId = -1;
            switch (cardId) {
                case 0: targetZoneId = 2; break; // Green card -> Zone B terminal
                case 1: targetZoneId = 3; break; // Purple card -> Zone C terminal
                case 2: targetZoneId = 4; break; // Red card -> Zone D terminal
                case 3: targetZoneId = 1; break; // Yellow card -> Zone A terminal
            }
            
            if (targetZoneId >= 1 && targetZoneId <= 4) {
                char zoneLetter = Game::ZoneIdToLetter(targetZoneId);
                std::string message = std::string("Press '") + zoneLetter + "' to toggle path to Zone " + zoneLetter + " Terminal";
                ShowHackStatus(message, 2.0f);
            }
        }
    }
    
    Engine::Color PlayerHUD::GetCardColor(int cardId) const
    {
        switch (cardId) {
            case 0: return Engine::Color(0.0f, 1.0f, 0.0f, 1.0f); // Green
            case 1: return Engine::Color(0.8f, 0.0f, 1.0f, 1.0f); // Purple
            case 2: return Engine::Color(1.0f, 0.0f, 0.0f, 1.0f); // Red  
            case 3: return Engine::Color(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
            default: return Engine::Color(1.0f, 1.0f, 1.0f, 1.0f); // White fallback
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
        
        // Determine which terminal to path to
        // Green (G) -> Zone B, Purple (P) -> Zone C, Red (R) -> Zone D, Yellow (Y) -> Zone A
        int targetZoneId = -1;
        switch (m_activePathCardId) {
            case 0: targetZoneId = 2; break; // Green card -> Zone B terminal
            case 1: targetZoneId = 3; break; // Purple card -> Zone C terminal
            case 2: targetZoneId = 4; break; // Red card -> Zone D terminal
            case 3: targetZoneId = 1; break; // Yellow card -> Zone A terminal
        }
        
        if (targetZoneId < 1 || targetZoneId > 4) return;
        
        Engine::Vector2 terminalPos = GetTerminalPosition(targetZoneId);
        
        // Check if terminal position is valid (not at origin)
        if (terminalPos.x == 0.0f && terminalPos.y == 0.0f) {
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
        pathRenderer->SetPath(currentPos, terminalPos, m_activePathCardId, GetCardColor(m_activePathCardId));
        
        // Get the waypoints and display them on the minimap with card color!
        if (m_minimap) {
            m_minimap->SetPathWaypoints(pathRenderer->pathWaypoints);
            m_minimap->SetPathColor(GetCardColor(m_activePathCardId)); // SET COLOR BASED ON CARD!
        }
    }

} // namespace Game