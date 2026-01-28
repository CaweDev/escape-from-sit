/**
 * @file Map.hpp
 * @brief UI element that displays a larger full-screen map view
 */
#pragma once

#include "UIElement.hpp"
#include "../TileMap/TileMap.hpp"
#include "../Core/Transform.hpp"
#include "../Graphics/Color.hpp"
#include "../Math/Mathf.hpp"

namespace UI
{
    /**
     * @class Map
     * @brief Renders a large top-down view of the entire tile map
     * 
     * Similar to Minimap but designed for full-screen viewing with
     * the entire map visible at once.
     */
    class Map : public UIElement
    {
    public:
        // Colors for rendering
        Engine::Color wallColor;
        Engine::Color floorColor;
        Engine::Color playerColor;
        Engine::Color backgroundColor;

    private:
        Engine::TileMap* m_tileMap = nullptr;
        Engine::Transform* m_playerTransform = nullptr;
        
        // Cached mesh for rendering
        AEGfxVertexList* m_quadMesh = nullptr;

    public:
        Map();
        virtual ~Map();

        /**
         * @brief Set the tile map to display
         * @param tileMap Pointer to the tile map (not owned)
         */
        void SetTileMap(Engine::TileMap* tileMap);
        Engine::TileMap* GetTileMap() const { return m_tileMap; }

        /**
         * @brief Set the player transform to track
         * @param transform Pointer to the player's transform (not owned)
         */
        void SetPlayerTransform(Engine::Transform* transform);
        Engine::Transform* GetPlayerTransform() const { return m_playerTransform; }

        /**
         * @brief Render the map
         */
        void Render() override;

    private:
        /**
         * @brief Initialize the quad mesh for rendering
         */
        void InitMesh();

        /**
         * @brief Draw a single colored quad
         * @param x World X position
         * @param y World Y position
         * @param width Width of the quad
         * @param height Height of the quad
         * @param color Color to draw
         */
        void DrawQuad(f32 x, f32 y, f32 width, f32 height, const Engine::Color& color);
    };

} // namespace UI
