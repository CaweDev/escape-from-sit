/**
 * @file Map.cpp
 * @brief Implementation of the full Map UI element
 */

#include "Map.hpp"
#include "AEEngine.h"

namespace UI
{
    Map::Map()
        : wallColor(0.5f, 0.5f, 0.6f, 1.0f)
        , floorColor(0.25f, 0.25f, 0.3f, 1.0f)
        , playerColor(0.0f, 1.0f, 0.3f, 1.0f)
        , backgroundColor(0.08f, 0.08f, 0.1f, 0.95f)
    {
        // Start hidden by default
        visible = false;
        InitMesh();
    }

    Map::~Map()
    {
        if (m_quadMesh)
        {
            AEGfxMeshFree(m_quadMesh);
            m_quadMesh = nullptr;
        }
    }

    void Map::InitMesh()
    {
        // Create a unit quad mesh (1x1 centered at origin)
        AEGfxMeshStart();
        AEGfxTriAdd(
            -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
            0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
            -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f
        );
        AEGfxTriAdd(
            0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
            0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
            -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f
        );
        m_quadMesh = AEGfxMeshEnd();
    }

    void Map::SetTileMap(Engine::TileMap* tileMap)
    {
        m_tileMap = tileMap;
    }

    void Map::SetPlayerTransform(Engine::Transform* transform)
    {
        m_playerTransform = transform;
    }

    void Map::DrawQuad(f32 x, f32 y, f32 width, f32 height, const Engine::Color& color)
    {
        if (!m_quadMesh) return;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(color.r, color.g, color.b, color.a);
        AEGfxSetTransparency(color.a);

        // Set transform: position and scale
        AEMtx33 transform;
        AEMtx33Scale(&transform, width, height);
        
        AEMtx33 translate;
        AEMtx33Trans(&translate, x, y);
        
        AEMtx33 final;
        AEMtx33Concat(&final, &translate, &transform);
        
        AEGfxSetTransform(final.m);
        AEGfxMeshDraw(m_quadMesh, AE_GFX_MDM_TRIANGLES);
    }

    void Map::Render()
    {
        if (!visible) return;

        Rect worldRect = GetWorldRect();
        f32 mapX = worldRect.x;
        f32 mapY = worldRect.y;
        f32 mapWidth = worldRect.width;
        f32 mapHeight = worldRect.height;

        // Draw background
        DrawQuad(mapX + mapWidth * 0.5f, mapY + mapHeight * 0.5f, mapWidth, mapHeight, backgroundColor);

        if (!m_tileMap) return;

        int mapTileWidth = m_tileMap->width;
        int mapTileHeight = m_tileMap->height;

        if (mapTileWidth <= 0 || mapTileHeight <= 0) return;

        // Calculate tile size to fit entire map within bounds
        f32 tileSizeX = mapWidth / static_cast<f32>(mapTileWidth);
        f32 tileSizeY = mapHeight / static_cast<f32>(mapTileHeight);
        f32 tileSize = Engine::Mathf::Min(tileSizeX, tileSizeY);

        // Calculate offset to center the map
        f32 totalWidth = mapTileWidth * tileSize;
        f32 totalHeight = mapTileHeight * tileSize;
        f32 offsetX = mapX + (mapWidth - totalWidth) * 0.5f;
        f32 offsetY = mapY + (mapHeight - totalHeight) * 0.5f;

        // Draw all tiles
        for (int ty = 0; ty < mapTileHeight; ++ty)
        {
            for (int tx = 0; tx < mapTileWidth; ++tx)
            {
                Engine::TileType tile = m_tileMap->GetTile(tx, ty);
                
                // Determine color based on tile type
                Engine::Color tileColor = floorColor;
                if (static_cast<int>(tile) > 0)
                {
                    tileColor = wallColor;
                }

                // Calculate screen position
                f32 screenX = offsetX + tx * tileSize + tileSize * 0.5f;
                f32 screenY = offsetY + ty * tileSize + tileSize * 0.5f;

                DrawQuad(screenX, screenY, tileSize - 0.5f, tileSize - 0.5f, tileColor);
            }
        }

        // Draw player position
        if (m_playerTransform)
        {
            f32 playerTileX = m_playerTransform->position.x;
            f32 playerTileY = m_playerTransform->position.y;

            // Clamp player position to map bounds
            playerTileX = Engine::Mathf::Clamp(playerTileX, 0.0f, static_cast<f32>(mapTileWidth));
            playerTileY = Engine::Mathf::Clamp(playerTileY, 0.0f, static_cast<f32>(mapTileHeight));

            f32 playerScreenX = offsetX + playerTileX * tileSize + tileSize * 0.5f;
            f32 playerScreenY = offsetY + playerTileY * tileSize + tileSize * 0.5f;

            // Draw player as a larger dot with glow effect
            f32 playerSize = tileSize * 1.2f;
            DrawQuad(playerScreenX, playerScreenY, playerSize, playerSize, playerColor);
        }
    }

} // namespace UI
