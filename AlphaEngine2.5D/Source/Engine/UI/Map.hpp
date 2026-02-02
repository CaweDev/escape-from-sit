/**
 * @file Map.hpp
 * @brief UI element that displays a larger full-screen map view
 */
#pragma once

#include "UIElement.hpp"
#include "../TileMap/TileMap.hpp"
#include "../Core/Transform.hpp"
#include "../Graphics/Color.hpp"
#include "../Graphics/Texture2D.hpp"
#include "../Math/Mathf.hpp"
#include "AEEngine.h"
#include <vector>
namespace UI
{
    using Engine::Texture2D;

    /**
     * @class Map
     * @brief Renders a large top-down view of the entire tile map
     * 
     * Designed for full-screen viewing. Unlike the Minimap, this class
     * scales the entire level to fit within its UI bounds. It supports
     * background textures, decorative borders, and displays the player's 
     * position as a marker.
     */
    class Map : public UIElement
    {
    private:
        // --- Rendering Colors ---
        Engine::Color m_wallColor;       // Color used for walls/unexplored areas
        Engine::Color m_floorColor;      // Color used for floor tiles
        Engine::Color m_playerColor;     // Color for the player marker
        Engine::Color m_backgroundColor; // Background panel color
        Engine::Color m_borderColor;     // Color for the decorative border frame
        Engine::Color m_enemyColor = Engine::Color(0.9f, 0.1f, 0.1f, 1.0f);
        // --- Configuration ---
        f32 m_borderThickness = 0.0f;    // Thickness of the border frame in pixels

        // --- Level & Player State ---
        Engine::TileMap* m_tileMap = nullptr;             // Level data source
        Engine::Transform* m_playerTransform = nullptr;   // Player tracking
        Texture2D* m_texture = nullptr;                  // Optional background texture
        std::vector<Engine::Transform*> m_enemyTransforms;
        // --- Graphics Resources ---
        AEGfxVertexList* m_quadMesh = nullptr;            // Cached unit quad mesh


    public:
        Map();
        virtual ~Map();

        /**
         * @brief Render the map
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
         * @brief Set the background texture for the map panel
         * @param pTexture Pointer to the texture (not owned)
         */
        void SetTexture(Texture2D* pTexture) { m_texture = pTexture; }
        void AddEnemyTransform(Engine::Transform* transform);
        void ClearEnemies() { m_enemyTransforms.clear(); }

        void SetEnemyColor(const Engine::Color& color) { m_enemyColor = color; }
        const Engine::Color& GetEnemyColor() const { return m_enemyColor; }

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

        // Getter and Setter for Border Thickness
        void SetBorderThickness(f32 thickness) { m_borderThickness = thickness; }
        f32 GetBorderThickness() const { return m_borderThickness; }

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
         * @brief Draw a single textured quad
         */
        void DrawQuadTextured(f32 x, f32 y, f32 width, f32 height, Texture2D* pTexture, const Engine::Color& color);
    };

} // namespace UI
