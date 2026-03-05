/**
 * @file HeatmapOverlay.cpp
 * @brief Implementation of HeatmapOverlay class
 */

#include "HeatmapOverlay.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/Math/Mathf.hpp"
#include <algorithm>

namespace UI
{
    HeatmapOverlay::HeatmapOverlay()
    {
        // Set default gradient: Blue (cold) -> Yellow (warm) -> Red (hot)
        m_coldColor = Engine::Color(0.0f, 0.0f, 1.0f, 0.0f);   // Blue, transparent initially
        m_warmColor = Engine::Color(1.0f, 1.0f, 0.0f, 0.7f);  // Yellow
        m_hotColor = Engine::Color(1.0f, 0.0f, 0.0f, 0.9f);   // Red
        
        // Set default background to semi-transparent dark
        SetBackgroundColor(Engine::Color(0.0f, 0.0f, 0.0f, 0.8f));
        
        // Make walls more visible
        SetWallColor(Engine::Color(0.3f, 0.3f, 0.3f, 1.0f));
        SetFloorColor(Engine::Color(0.1f, 0.1f, 0.1f, 0.5f));
    }

    HeatmapOverlay::~HeatmapOverlay()
    {
        // Base class handles mesh cleanup
    }

    void HeatmapOverlay::Render()
    {
        if (!visible) return;
        if (!m_heatmap || !GetTileMap()) return;

        // First render the base map (walls, floor, player, etc.)
        Map::Render();

        // Now render heatmap overlay on top
        auto tileMap = GetTileMap();
        int mapWidth = tileMap->width;
        int mapHeight = tileMap->height;

        // Get the pixel dimensions of the map UI component
        Rect worldRect = GetWorldRect();
        f32 mapX = worldRect.x;
        f32 mapY = worldRect.y;
        f32 mapRectWidth = worldRect.width;
        f32 mapRectHeight = worldRect.height;

        // Calculate inner area (accounting for border)
        f32 borderThickness = GetBorderThickness();
        f32 innerWidth = mapRectWidth - 2.0f * borderThickness;
        f32 innerHeight = mapRectHeight - 2.0f * borderThickness;
        f32 innerX = mapX + borderThickness;
        f32 innerY = mapY + borderThickness;

        // Calculate uniform tile size
        f32 tileSizeX = innerWidth / static_cast<f32>(mapWidth);
        f32 tileSizeY = innerHeight / static_cast<f32>(mapHeight);
        f32 tileSize = Engine::Mathf::Min(tileSizeX, tileSizeY);

        // Center the tile grid
        f32 totalWidth = mapWidth * tileSize;
        f32 totalHeight = mapHeight * tileSize;
        f32 offsetX = innerX + (innerWidth - totalWidth) * 0.5f;
        f32 offsetY = innerY + (innerHeight - totalHeight) * 0.5f;

        // Dynamically calculate max heat value if needed
        float dynamicMaxValue = m_heatmap->MaxValue();
        if (dynamicMaxValue > m_maxValue) {
            m_maxValue = dynamicMaxValue;
        }

        // Render heatmap overlay
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                float heatValue = m_heatmap->Get(x, y);
                
                // Skip tiles below threshold
                if (heatValue < m_minThreshold) continue;

                // Get color based on heat value
                Engine::Color heatColor = GetHeatColor(heatValue);
                
                // Apply global opacity
                heatColor.a *= m_opacity;

                // Calculate tile position in screen space
                f32 tileX = offsetX + x * tileSize + tileSize * 0.5f;
                f32 tileY = offsetY + y * tileSize + tileSize * 0.5f;

                // Draw colored quad
                DrawQuad(tileX, tileY, tileSize, tileSize, heatColor);
            }
        }
    }

    void HeatmapOverlay::SetHeatGradient(const Engine::Color& cold, const Engine::Color& warm, const Engine::Color& hot)
    {
        m_coldColor = cold;
        m_warmColor = warm;
        m_hotColor = hot;
    }

    Engine::Color HeatmapOverlay::GetHeatColor(float heatValue) const
    {
        // Normalize heat value to [0, 1]
        float normalized = Engine::Mathf::Clamp01(heatValue / m_maxValue);

        // Three-step gradient: cold -> warm -> hot
        if (normalized < 0.5f) {
            // Interpolate between cold and warm
            float t = normalized * 2.0f; // Map [0, 0.5] to [0, 1]
            return LerpColor(m_coldColor, m_warmColor, t);
        } else {
            // Interpolate between warm and hot
            float t = (normalized - 0.5f) * 2.0f; // Map [0.5, 1] to [0, 1]
            return LerpColor(m_warmColor, m_hotColor, t);
        }
    }

    Engine::Color HeatmapOverlay::LerpColor(const Engine::Color& a, const Engine::Color& b, float t) const
    {
        t = Engine::Mathf::Clamp01(t);
        return Engine::Color(
            Engine::Mathf::Lerp(a.r, b.r, t),
            Engine::Mathf::Lerp(a.g, b.g, t),
            Engine::Mathf::Lerp(a.b, b.b, t),
            Engine::Mathf::Lerp(a.a, b.a, t)
        );
    }

} // namespace UI
