/**
 * @file PlayerHUD.hpp
 * @brief Component that manages the player's HUD including Minimap and Map
 */
#pragma once

#include "Engine/Core/Component.hpp"
#include "Engine/UI/Core/Canvas.hpp"
#include "Engine/UI/Specialized/Minimap.hpp"
#include "Engine/UI/Specialized/Map.hpp"
#include "Engine/UI/Specialized/HeatmapOverlay.hpp"
#include "Engine/UI/Components/Panel.hpp"
#include "Engine/UI/Components/Text.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include "Engine/TileMap/Door.hpp"

#include <vector>
#include <functional>
#include "Game/ucb1bandit/BanditVisualizer.hpp"
namespace Game
{
    /**
     * @class PlayerHUD
     * @brief Component that creates and manages Minimap and Map UI elements
     *
     * Attach this to a GameObject (typically the Player) to get:
     * - A corner minimap showing nearby tiles
     * - A toggleable full map (M key)
     */
    class PlayerHUD : public Engine::Component
    {
    private:
        // Minimap settings
        f32 m_minimapWidth = 180.0f;
        f32 m_minimapHeight = 180.0f;
        f32 m_minimapMargin = 20.0f;

        // Full map settings
        f32 m_mapWidth = 500.0f;
        f32 m_mapHeight = 500.0f;

        UI::Canvas* m_canvas = nullptr;
        UI::Minimap* m_minimap = nullptr;
        UI::Map* m_map = nullptr;
        UI::HeatmapOverlay* m_heatmapOverlay = nullptr; // Heatmap visualization

        // Recommendation 2: Alert Level UI
        UI::Panel* m_alertPanel = nullptr;
        UI::Text* m_alertText = nullptr;
        UI::Panel* m_alertBar = nullptr;
        UI::Text* m_zoneInfoText = nullptr;

        // CCTV warning indicator
        UI::Panel* m_cctvWarningPanel = nullptr;
        UI::Text* m_cctvWarningText = nullptr;
        float m_cctvWarningTimer = 0.0f;
        float m_cctvWarningDuration = 2.0f; // Show warning for 2 seconds
        
        // Stamina bar
        UI::Panel* m_staminaPanel = nullptr;
        UI::Panel* m_staminaBar = nullptr;
        UI::Text* m_staminaText = nullptr;
        
        // High Score display
        UI::Panel* m_scorePanel = nullptr;
        UI::Text* m_highScoreText = nullptr;
        UI::Text* m_currentTimeText = nullptr;
        
        // Guard proximity warning
        UI::Text* m_proximityText = nullptr;
        float m_proximityIntensity = 0.0f;
        
        // NPC approach indicators (Annoying + Bully)
        UI::Text* m_annoyingApproachText = nullptr;
        UI::Text* m_bullyApproachText = nullptr;
        float m_annoyingApproachTimer = 0.0f;
        float m_bullyApproachTimer = 0.0f;
        float m_approachDisplayDuration = 3.0f; // seconds to show approach messages
        // (no SFX for NPC approach)
        
        // Hack status
        UI::Text* m_hackStatusText = nullptr;
        float m_hackStatusTimer = 0.0f;
        
        // Zone entry notification
        UI::Panel* m_zoneEntryPanel = nullptr;
        UI::Text* m_zoneEntryText = nullptr;
        float m_zoneEntryTimer = 0.0f;
        float m_zoneEntryDuration = 3.0f;
        int m_currentZone = -1; // Track current zone to detect changes

        Engine::TileMap* m_tileMap = nullptr;
        Engine::Transform* m_playerTransform = nullptr;

        bool m_mapVisible = false;
        bool m_heatmapVisible = false; // Track heatmap overlay visibility
        std::vector<Engine::Transform*> m_enemyTransforms;
        std::vector<Engine::Transform*> m_cctvTransforms;
        std::vector<Engine::Door*> m_doors;
        std::function<int(int, int)> m_zoneGetter;

        //BookCount
        UI::Panel* m_bookCountPanel = nullptr;
        UI::Text* m_bookCountText = nullptr;

        //Title
        UI::Panel* m_bookPopupPanel = nullptr;
        UI::Text* m_bookTitleText = nullptr;
        UI::Text* m_bookLoreText = nullptr;

        //Question Answers
        UI::Text* m_option1Text = nullptr;
        UI::Text* m_option2Text = nullptr;
        UI::Text* m_option3Text = nullptr;
        
        // Access Card tracking
        UI::Panel* m_cardPanel = nullptr;
        UI::Text* m_cardText = nullptr;
        bool m_cardsCollected[4] = {false, false, false, false}; // Track 4 access cards
        int m_totalCardsCollected = 0;
        
        // Terminal positions by zone (set by FPSScene)
        Engine::Vector2 m_terminalPositions[4]; // Zone 1-4 terminals (index 0-3)
        
        // Control room door positions by zone (set by FPSScene)
        Engine::Vector2 m_controlRoomPositions[4]; // Zone 1-4 control rooms (index 0-3)
        
        // Active path tracking
        int m_activePathCardId = -1; // Which card's path is currently shown (-1 = none)
        bool m_pathVisible = false; // Is path currently being rendered

        // Power-up HUD
        UI::Panel* m_powerUpPanel = nullptr;
        UI::Image* m_powerUpIcons[4] = { nullptr, nullptr, nullptr, nullptr };
        UI::Text* m_powerUpTimers[4] = { nullptr, nullptr, nullptr, nullptr };
        Engine::Texture2D* m_powerUpTextures[4] = { nullptr, nullptr, nullptr, nullptr };

        UI::Panel *inventoryPanel = nullptr;
        UI::Panel *inventorySlots[3] = {nullptr, nullptr, nullptr};
        UI::Image *inventoryIcons[3] = {nullptr, nullptr, nullptr};

        Game::BanditVisualizer* m_banditVisualizer = nullptr;
    public:
        PlayerHUD() = default;
        virtual ~PlayerHUD();

        // NPC approach indicators (Annoying + Bully)
        void UpdateNPCApproach(int annoyingCount, float nearestAnnoyingDist, int bullyCount, float nearestBullyDist);
        void SetBanditVisualizer(Game::BanditVisualizer* bv) { m_banditVisualizer = bv; }
        /**
         * @brief Called when the component is created
         */
        void Awake() override;

        /**
         * @brief Called every frame
         */
        void Update() override;

        /**
         * @brief Called when the component is destroyed
         */
        void OnDestroy() override;

        /**
         * @brief Set the tile map reference for both minimap and map
         * @param tileMap Pointer to the tile map (not owned)
         */
        void SetTileMap(Engine::TileMap* tileMap);
        Engine::TileMap* GetTileMap() const { return m_tileMap; }
        void SetZoneGetter(std::function<int(int, int)> fn);
        /**
         * @brief Set the player transform for position tracking
         * @param transform Pointer to player transform (not owned)
         */
        void SetPlayerTransform(Engine::Transform* transform);
        Engine::Transform* GetPlayerTransform() const { return m_playerTransform; }

        UI::Minimap* GetMinimap() const { return m_minimap; }
        void AddEnemyTransform(Engine::Transform* enemyTransform);


        void AddCCTVTransform(Engine::Transform* cctvTransform);
        void ClearCCTVs();
        void AddDoor(Engine::Door *door);

        /**
         * @brief Toggle the full map visibility
         */
        void ToggleMap();
        
        /**
         * @brief Toggle the heatmap overlay visibility
         */
        void ToggleHeatmap();

        /**
         * @brief Check if the full map is visible
         */
        bool IsMapVisible() const { return m_mapVisible; }
        
        /**
         * @brief Check if the heatmap overlay is visible
         */
        bool IsHeatmapVisible() const { return m_heatmapVisible; }

        /**
         * @brief Show the full map
         */
        void ShowMap();

        /**
         * @brief Hide the full map
         */
        void HideMap();
        
        /**
         * @brief Show the heatmap overlay
         */
        void ShowHeatmap();

        /**
         * @brief Hide the heatmap overlay
         */
        void HideHeatmap();
        
        /**
         * @brief Set the heatmap data source
         * @param heatmap Pointer to heatmap (not owned)
         */
        void SetHeatmap(class Heatmap* heatmap);

        /**
         * @brief Get the minimap element
         */


         /**
          * @brief Get the map element
          */
        UI::Map* GetMap() const { return m_map; }

        // Getters and Setters for Minimap settings
        void SetMinimapWidth(f32 width) { m_minimapWidth = width; }
        f32 GetMinimapWidth() const { return m_minimapWidth; }

        void SetMinimapHeight(f32 height) { m_minimapHeight = height; }
        f32 GetMinimapHeight() const { return m_minimapHeight; }

        void SetMinimapMargin(f32 margin) { m_minimapMargin = margin; }
        f32 GetMinimapMargin() const { return m_minimapMargin; }

        // Getters and Setters for Map settings
        void SetMapWidth(f32 width) { m_mapWidth = width; }
        f32 GetMapWidth() const { return m_mapWidth; }

        void SetMapHeight(f32 height) { m_mapHeight = height; }
        f32 GetMapHeight() const { return m_mapHeight; }

        // Recommendation 2: Alert Level UI Controls
        void UpdateAlertLevel(int stage, int priorityZone, int guardCount);
        void SetAlertUIVisible(bool visible);
        void ShowCCTVWarning(bool show); // Flash warning when spotted by CCTV
        void SetAlertDetailMode(bool detailed); // Toggle between simple/detailed view
        
        // Stamina bar controls
        void UpdateStamina(float current, float max);
        
        // High score controls
        void UpdateHighScore(float highScore);
        void UpdateCurrentTime(float currentTime);
        
        // Guard proximity warning
        void UpdateGuardProximity(float nearestDistance);
        
        // Hack status feedback
        void ShowHackStatus(const std::string& message, float duration = 3.0f);
        
        // Zone entry notification
        void CheckZoneChange(int zoneId); // Call this every frame with current zone
        void ShowZoneEntry(int zoneId); // Show "Entering Zone X" message
        
        // Access card collection
        void CollectAccessCard(int cardId, const Engine::Vector2& collectionPos); // Called when player collects a card (0-3)
        bool HasAccessCard(int cardId) const; // Check if specific card is collected
        int GetCollectedCardCount() const { return m_totalCardsCollected; }
        bool HasAllAccessCards() const { return m_totalCardsCollected >= 4; }
        void UpdateCardDisplay(); // Update UI to show collected cards
        
        // Terminal positions (set by FPSScene for pathfinding)
        void SetTerminalPosition(int zoneId, const Engine::Vector2& position); // Zone 1-4
        Engine::Vector2 GetTerminalPosition(int zoneId) const; // Zone 1-4
        
        // Control room door positions (set by FPSScene for pathfinding)
        void SetControlRoomPosition(int zoneId, const Engine::Vector2& position); // Zone 1-4
        Engine::Vector2 GetControlRoomPosition(int zoneId) const; // Zone 1-4
        
        // Path control (press key to toggle)
        void TogglePath(int cardId); // Toggle path for specific card (0-3)
        void UpdateActivePath(); // Update path from current position each frame
        
        // Get color for each access card (for minimap path coloring)
        Engine::Color GetCardColor(int cardId) const;

        void UpdatePowerUpUI();

        void UpdateInventory();
    };

} // namespace Game