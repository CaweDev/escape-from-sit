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
#include <vector>

namespace UI
{
    /**
     * @struct MinimapVertex
     * @brief Vertex structure for dynamic minimap geometry
     * 
     * Used for building custom meshes on the fly, specifically for the
     * Sutherland-Hodgman clipping implementation that keeps the minimap
     * within its circular or rectangular bounds.
     */
    struct MinimapVertex
    {
        AEVec2 pos;     // Screen-space position
        u32 color;      // Packed 32-bit color (RGBA)
        AEVec2 uv;      // Texture coordinates for sprites
    };

    /**
     * @class Minimap
     * @brief Renders a small top-down view of the tile map with player position
     * 
     * This element tracks the player's position and renders a local neighborhood
     * of tiles. It supports arbitrary rotation (tracking player heading) and
     * uses a custom clipping algorithm to ensure tiles don't overflow the UI bounds.
     */
    class Minimap : public UIElement
    {
    private:
        // --- Rendering Colors ---
        Engine::Color m_wallColor;       // Color used for wall tiles
        Engine::Color m_floorColor;      // Color used for walkable floor tiles
        Engine::Color m_playerColor;     // Color for the player icon/dot
        Engine::Color m_backgroundColor; // Background panel color (visible behind tiles)
        Engine::Color m_borderColor;     // Color for the decorative border frame
                      
        Engine::Color m_enemyColor = Engine::Color(0.9f, 0.1f, 0.1f, 1.0f); ;
        // --- View Configuration ---
        int m_viewRadius = 5;            // How many tiles to show in each direction
        f32 m_tilePixelSize = 8.0f;       // Base size of a single tile in pixels
        bool m_centerOnPlayer = true;    // If true, the map scrolls as the player moves
        f32 m_borderThickness = 8.0f;     // Thickness of the border used for clipping/masking

        // --- Component References ---
        Engine::TileMap* m_tileMap = nullptr;             // Pointer to the level data
        Engine::Transform* m_playerTransform = nullptr;   // Pointer to player transform for tracking
        std::vector<Engine::Transform*> m_enemyTransforms;
        std::vector<Engine::Transform*> m_cctvTransforms;
        Engine::Sprite* m_cctvSprite = nullptr;
        // --- Graphics Resources ---
        AEGfxVertexList* m_quadMesh = nullptr;            // Cached unit quad mesh for simple draws

        // --- Optional Sprites ---
        std::shared_ptr<Engine::Sprite> m_playerSprite = nullptr; // Optional icon for the player
        std::shared_ptr<Engine::Sprite> m_borderSprite = nullptr; // Optional texture for the border frame

    public:
        Minimap();
        virtual ~Minimap();

        /**
         * @brief Render the minimap
         */
        void Render() override;

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

        void AddEnemyTransform(Engine::Transform* transform);
        void ClearEnemies() { m_enemyTransforms.clear(); }

        void SetEnemyColor(const Engine::Color& color) { m_enemyColor = color; }
        const Engine::Color& GetEnemyColor() const { return m_enemyColor; }
        void AddCCTVTransform(Engine::Transform* transform);
        void ClearCCTVs();
        void SetCCTVSprites(Engine::Sprite* sprite);
        // Getters and Setters for Colors
        void SetWallColor(const Engine::Color& color) { m_wallColor = color; }
        const Engine::Color& GetWallColor() const { return m_wallColor; }

        void SetFloorColor(const Engine::Color& color) { m_floorColor = color; }
        const Engine::Color& GetFloorColor() const { return m_floorColor; }

        void SetPlayerColor(const Engine::Color& color) { m_playerColor = color; }
        const Engine::Color& GetPlayerColor() const { return m_playerColor; }

        void SetBackgroundColor(const Engine::Color& color) { m_backgroundColor = color; }
        const Engine::Color& GetBackgroundColor() const { return m_backgroundColor; }

        void SetBorderColor(const Engine::Color& color) { m_borderColor = color; }
        const Engine::Color& GetBorderColor() const { return m_borderColor; }

        // Getters and Setters for Configuration
        void SetViewRadius(int radius) { m_viewRadius = radius; }
        int GetViewRadius() const { return m_viewRadius; }

        void SetTilePixelSize(f32 size) { m_tilePixelSize = size; }
        f32 GetTilePixelSize() const { return m_tilePixelSize; }

        void SetCenterOnPlayer(bool center) { m_centerOnPlayer = center; }
        bool GetCenterOnPlayer() const { return m_centerOnPlayer; }

        void SetBorderThickness(f32 thickness) { m_borderThickness = thickness; }
        f32 GetBorderThickness() const { return m_borderThickness; }

    private:
        /**
         * @brief Initialize the quad mesh for rendering
         */
        void InitMesh();

        /**
         * @brief Get vertices for a quad clipped to the minimap bounds
         */
        std::vector<MinimapVertex> GetClippedVertices(f32 x, f32 y, f32 width, f32 height, f32 rotationRad, const Engine::Color& color);

        /**
         * @brief Draw a quad clipped to the minimap bounds (standalone, for icon)
         */
        void DrawQuadClipped(f32 x, f32 y, f32 width, f32 height, f32 rotationRad, const Engine::Color& color, Engine::Sprite* sprite = nullptr);

        /**
         * @brief Draw border frame around the minimap
         */
        void DrawBorderFrame(const Rect& mapRect);
    };

} // namespace UI
