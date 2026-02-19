/**
 * @file Map.cpp
 * @brief Implementation of the full Map UI element
 */

#include "Engine/UI/Specialized/Map.hpp"
#include "AEEngine.h"

namespace UI
{
    Map::Map()
        : m_wallColor(0.5f, 0.5f, 0.6f, 1.0f)
        , m_floorColor(0.25f, 0.25f, 0.3f, 1.0f)
        , m_playerColor(0.0f, 1.0f, 0.3f, 1.0f)
        , m_backgroundColor(0.08f, 0.08f, 0.1f, 0.95f)
        , m_borderColor(1.0f, 1.0f, 1.0f, 1.0f)
        , m_borderThickness(10.0f)
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
    void Map::AddEnemyTransform(Engine::Transform* transform)
    {
        if (!transform) return;
        m_enemyTransforms.push_back(transform);
    }
    void Map::AddCCTVTransform(Engine::Transform* transform)
    {
        if (!transform) return;
        m_cctvTransforms.push_back(transform);
    }

    void Map::ClearCCTVs() { m_cctvTransforms.clear(); }
    void Map::SetCCTVSprites(Engine::Sprite* sprite) { m_cctvSprite = sprite; }
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

    void Map::DrawQuadTextured(f32 x, f32 y, f32 width, f32 height, Texture2D* pTexture, const Engine::Color& color)
    {
        if (!m_quadMesh || !pTexture || !pTexture->IsLoaded()) return;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxTextureSet(pTexture->GetNativeTexture(), 0.0f, 0.0f);
        AEGfxSetColorToMultiply(color.r, color.g, color.b, color.a);
        AEGfxSetTransparency(color.a);

        AEMtx33 scale, translate, final;
        AEMtx33Scale(&scale, width, height);
        AEMtx33Trans(&translate, x, y);
        AEMtx33Concat(&final, &translate, &scale);
        
        AEGfxSetTransform(final.m);
        AEGfxMeshDraw(m_quadMesh, AE_GFX_MDM_TRIANGLES);
    }
    void UI::Map::DrawQuadSprite(f32 x, f32 y, f32 width, f32 height,
        Engine::Sprite* sprite,
        const Engine::Color& color)
    {
        if (!m_quadMesh || !sprite) return;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxTextureSet(sprite->GetTexture(), 0.0f, 0.0f);
        AEGfxSetColorToMultiply(color.r, color.g, color.b, color.a);
        AEGfxSetTransparency(color.a);

        AEMtx33 scale, translate, finalMtx;
        AEMtx33Scale(&scale, width, height);
        AEMtx33Trans(&translate, x, y);
        AEMtx33Concat(&finalMtx, &translate, &scale);

        AEGfxSetTransform(finalMtx.m);
        AEGfxMeshDraw(m_quadMesh, AE_GFX_MDM_TRIANGLES);
    }

    void Map::Render()
    {
        // Don't render if hidden (visible is a property of UIElement)
        if (!visible) return;

        // Get the pixel dimensions of the map UI component
        Rect worldRect = GetWorldRect();
        f32 mapX = worldRect.x;
        f32 mapY = worldRect.y;
        f32 mapWidth = worldRect.width;
        f32 mapHeight = worldRect.height;
        f32 centerX = mapX + mapWidth * 0.5f;
        f32 centerY = mapY + mapHeight * 0.5f;

        // --- Step 1: Draw the Map Panel/Background ---
        // If a texture is assigned, we draw that (e.g., a paper map texture).
        // Otherwise, we draw a solid rectangle using m_wallColor to represent the "void".
        if (m_texture)
        {
            DrawQuadTextured(centerX, centerY, mapWidth, mapHeight, m_texture, m_wallColor);
        }
        else
        {
            DrawQuad(centerX, centerY, mapWidth, mapHeight, m_wallColor);
        }

        // --- Step 2: Decorative Border ---
        // Draw 4 rectangles around the edges if a border thickness is set.
        // We only do this if no background texture is present, assuming the texture
        // already includes its own border aesthetic.
        if (m_borderThickness > 0.0f && !m_texture)
        {
            f32 t = m_borderThickness;
            // Left
            DrawQuad(mapX + t * 0.5f, centerY, t, mapHeight, m_borderColor);
            // Right
            DrawQuad(mapX + mapWidth - t * 0.5f, centerY, t, mapHeight, m_borderColor);
            // Top
            DrawQuad(centerX, mapY + mapHeight - t * 0.5f, mapWidth, t, m_borderColor);
            // Bottom
            DrawQuad(centerX, mapY + t * 0.5f, mapWidth, t, m_borderColor);
        }

        if (!m_tileMap) return;

        // --- Step 3: Calculate Layout for the Entire Level ---
        int mapTileWidth = m_tileMap->width;
        int mapTileHeight = m_tileMap->height;

        if (mapTileWidth <= 0 || mapTileHeight <= 0) return;

        // Determine the "inner area" where tiles will actually be drawn
        f32 innerWidth = mapWidth - 2.0f * m_borderThickness;
        f32 innerHeight = mapHeight - 2.0f * m_borderThickness;
        f32 innerX = mapX + m_borderThickness;
        f32 innerY = mapY + m_borderThickness;

        // Calculate a uniform tile size that fits the whole map while maintaining aspect ratio
        f32 tileSizeX = innerWidth / static_cast<f32>(mapTileWidth);
        f32 tileSizeY = innerHeight / static_cast<f32>(mapTileHeight);
        f32 tileSize = Engine::Mathf::Min(tileSizeX, tileSizeY);

        // Center the fitted tile grid within the map element's bounds
        f32 totalWidth = mapTileWidth * tileSize;
        f32 totalHeight = mapTileHeight * tileSize;
        f32 offsetX = innerX + (innerWidth - totalWidth) * 0.5f;
        f32 offsetY = innerY + (innerHeight - totalHeight) * 0.5f;

        // --- Step 4: Logic Loop to Draw Tiles ---
        for (int ty = 0; ty < mapTileHeight; ++ty)
        {
            for (int tx = 0; tx < mapTileWidth; ++tx)
            {
                Engine::TileType tile = m_tileMap->GetTile(tx, ty);
                int itype = static_cast<int>(tile);

                // Optimization: Skip rendering walls (any tile with index > 0)
                // because we already filled the background with wallColor in Step 1.
                if (itype > 0) continue;

                // Floors draw with m_floorColor
                Engine::Color tileColor = m_floorColor;

                // Calculate the screen position for this specific tile
                f32 screenX = offsetX + tx * tileSize + tileSize * 0.5f;
                f32 screenY = offsetY + ty * tileSize + tileSize * 0.5f;

                // Render the tile as a simple solid quad (seamlessly touching neighbors)
                DrawQuad(screenX, screenY, tileSize, tileSize, tileColor);
            }
        }
        {
            f32 sz = tileSize * 1.05f;

            for (auto* tf : m_cctvTransforms)
            {
                if (!tf) continue;

                f32 x = Engine::Mathf::Clamp(tf->position.x, 0.0f, (f32)mapTileWidth);
                f32 y = Engine::Mathf::Clamp(tf->position.y, 0.0f, (f32)mapTileHeight);

                f32 sx = offsetX + x * tileSize;
                f32 sy = offsetY + y * tileSize;

                if (m_cctvSprite) DrawQuadSprite(sx, sy, sz, sz, m_cctvSprite, Engine::Color(1, 1, 1, 0.90f));
                else              DrawQuad(sx, sy, sz, sz, m_cctvColor);
            }
        }

        {
            f32 enemySize = tileSize * 1.2f;

            for (auto* enemyTf : m_enemyTransforms)
            {
                if (!enemyTf) continue;

                f32 ex = enemyTf->position.x;
                f32 ey = enemyTf->position.y;

                ex = Engine::Mathf::Clamp(ex, 0.0f, static_cast<f32>(mapTileWidth));
                ey = Engine::Mathf::Clamp(ey, 0.0f, static_cast<f32>(mapTileHeight));

                f32 enemyScreenX = offsetX + ex * tileSize + tileSize * 0.5f;
                f32 enemyScreenY = offsetY + ey * tileSize + tileSize * 0.5f;

                DrawQuad(enemyScreenX, enemyScreenY, enemySize, enemySize, m_enemyColor);
            }
        }

        // --- Step 5: Draw Player Position Marker ---
        if (m_playerTransform)
        {
            f32 playerTileX = m_playerTransform->position.x;
            f32 playerTileY = m_playerTransform->position.y;

            // Ensure the marker stays visible even if the player goes slightly out of bounds
            playerTileX = Engine::Mathf::Clamp(playerTileX, 0.0f, static_cast<f32>(mapTileWidth));
            playerTileY = Engine::Mathf::Clamp(playerTileY, 0.0f, static_cast<f32>(mapTileHeight));

            f32 playerScreenX = offsetX + playerTileX * tileSize;
            f32 playerScreenY = offsetY + playerTileY * tileSize;

            // Draw the player as a slightly larger contrasting dot
            f32 playerSize = tileSize * 1.5f;
            DrawQuad(playerScreenX, playerScreenY, playerSize, playerSize, m_playerColor);
        }

        // Render children
        UIElement::Render();
    }

} // namespace UI
