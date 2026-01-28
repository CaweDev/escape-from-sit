/**
 * @file Minimap.hpp
 * @brief UI element that displays a top-down minimap view of the tile map
 */
#pragma once

#include "UIElement.hpp"
#include "../TileMap/TileMap.hpp"
#include "../Core/Transform.hpp"
#include "../Graphics/Color.hpp"
#include "../Graphics/Sprite.hpp"
#include "../Math/Mathf.hpp"
#include <memory>

namespace UI
{
    /**
     * @class Minimap
     * @brief Renders a small top-down view of the tile map with player position
     * 
     * Displays walls and floors as colored rectangles and shows
     * the player's current position as a contrasting dot.
     */
    class Minimap : public UIElement
    {
    public:
        // Colors for rendering
        Engine::Color wallColor;
        Engine::Color floorColor;
        Engine::Color playerColor;
        Engine::Color backgroundColor;
        Engine::Color borderColor;

        // Scale: how many pixels per tile
        f32 tilePixelSize = 8.0f;

        // Whether to center the map on the player (scrolling minimap)
        bool centerOnPlayer = true;

        // Visible tile radius when centered on player
        int viewRadius = 10;

        // Border thickness for masking
        f32 borderThickness = 8.0f;

    private:
        Engine::TileMap* m_tileMap = nullptr;
        Engine::Transform* m_playerTransform = nullptr;
        
        // Cached mesh for rendering
        AEGfxVertexList* m_quadMesh = nullptr;

        // Optional textures
        std::shared_ptr<Engine::Sprite> m_playerSprite = nullptr;
        std::shared_ptr<Engine::Sprite> m_borderSprite = nullptr;

    public:
        Minimap();
        virtual ~Minimap();

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
         * @brief Set an optional sprite for the player icon
         * @param sprite Shared pointer to the sprite
         */
        void SetPlayerSprite(std::shared_ptr<Engine::Sprite> sprite) { m_playerSprite = sprite; }

        /**
         * @brief Set an optional sprite for the border
         * @param sprite Shared pointer to the sprite
         */
        void SetBorderSprite(std::shared_ptr<Engine::Sprite> sprite) { m_borderSprite = sprite; }

        /**
         * @brief Render the minimap
         */
        void Render() override;

    private:
        /**
         * @brief Initialize the quad mesh for rendering
         */
        void InitMesh();

        /**
         * @brief Draw a single colored quad
         */
        void DrawQuad(f32 x, f32 y, f32 width, f32 height, const Engine::Color& color);

        /**
         * @brief Draw a textured quad
         */
        void DrawQuadTextured(f32 x, f32 y, f32 width, f32 height, Engine::Sprite* sprite, const Engine::Color& tint);

        /**
         * @brief Draw a rotated colored quad
         */
        void DrawQuadRotated(f32 x, f32 y, f32 width, f32 height, f32 rotationRad, const Engine::Color& color);

        /**
         * @brief Draw border frame around the minimap
         */
        void DrawBorderFrame(const Rect& mapRect);

        /**
         * @brief Draw mask borders to cover overflow
         */
        void DrawMaskBorders(const Rect& mapRect, const Engine::Color& bgColor);
    };

} // namespace UI
