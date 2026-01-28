/**
 * @file PlayerHUD.hpp
 * @brief Component that manages the player's HUD including Minimap and Map
 */
#pragma once

#include "Engine/Core/Component.hpp"
#include "Engine/UI/Canvas.hpp"
#include "Engine/UI/Minimap.hpp"
#include "Engine/UI/Map.hpp"
#include "Engine/TileMap/TileMap.hpp"

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
    public:
        // Minimap settings
        f32 minimapWidth = 180.0f;
        f32 minimapHeight = 180.0f;
        f32 minimapMargin = 20.0f;

        // Full map settings
        f32 mapWidth = 500.0f;
        f32 mapHeight = 500.0f;

    private:
        UI::Canvas* m_canvas = nullptr;
        UI::Minimap* m_minimap = nullptr;
        UI::Map* m_map = nullptr;
        
        Engine::TileMap* m_tileMap = nullptr;
        Engine::Transform* m_playerTransform = nullptr;

        bool m_mapVisible = false;

    public:
        PlayerHUD() = default;
        virtual ~PlayerHUD();

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

        /**
         * @brief Set the player transform for position tracking
         * @param transform Pointer to player transform (not owned)
         */
        void SetPlayerTransform(Engine::Transform* transform);
        Engine::Transform* GetPlayerTransform() const { return m_playerTransform; }

        /**
         * @brief Toggle the full map visibility
         */
        void ToggleMap();

        /**
         * @brief Check if the full map is visible
         */
        bool IsMapVisible() const { return m_mapVisible; }

        /**
         * @brief Show the full map
         */
        void ShowMap();

        /**
         * @brief Hide the full map
         */
        void HideMap();

        /**
         * @brief Get the minimap element
         */
        UI::Minimap* GetMinimap() const { return m_minimap; }

        /**
         * @brief Get the map element
         */
        UI::Map* GetMap() const { return m_map; }
    };

} // namespace Game
