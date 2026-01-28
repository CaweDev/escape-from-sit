/**
 * @file Minimap.cpp
 * @brief Implementation of the Minimap UI element
 */

#include "Minimap.hpp"
#include "AEEngine.h"

namespace UI
{
    Minimap::Minimap()
        : wallColor(0.4f, 0.4f, 0.5f, 1.0f)
        , floorColor(0.2f, 0.2f, 0.25f, 1.0f)
        , playerColor(0.0f, 0.8f, 0.2f, 1.0f)
        , backgroundColor(0.1f, 0.1f, 0.12f, 0.85f)
        , borderColor(0.3f, 0.3f, 0.35f, 1.0f)
    {
        InitMesh();
    }

    Minimap::~Minimap()
    {
        if (m_quadMesh)
        {
            AEGfxMeshFree(m_quadMesh);
            m_quadMesh = nullptr;
        }
    }

    void Minimap::InitMesh()
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

    void Minimap::SetTileMap(Engine::TileMap* tileMap)
    {
        m_tileMap = tileMap;
    }

    void Minimap::SetPlayerTransform(Engine::Transform* transform)
    {
        m_playerTransform = transform;
    }

    void Minimap::DrawQuad(f32 x, f32 y, f32 width, f32 height, const Engine::Color& color)
    {
        if (!m_quadMesh) return;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(color.r, color.g, color.b, color.a);
        AEGfxSetTransparency(color.a);

        AEMtx33 transform;
        AEMtx33Scale(&transform, width, height);
        
        AEMtx33 translate;
        AEMtx33Trans(&translate, x, y);
        
        AEMtx33 finalMtx;
        AEMtx33Concat(&finalMtx, &translate, &transform);
        
        AEGfxSetTransform(finalMtx.m);
        AEGfxMeshDraw(m_quadMesh, AE_GFX_MDM_TRIANGLES);
    }

    void Minimap::DrawQuadTextured(f32 x, f32 y, f32 width, f32 height, Engine::Sprite* sprite, const Engine::Color& tint)
    {
        if (!m_quadMesh || !sprite || !sprite->GetTexture()) return;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxTextureSet(sprite->GetTexture(), 0.0f, 0.0f);
        AEGfxSetColorToMultiply(tint.r, tint.g, tint.b, tint.a);
        AEGfxSetTransparency(tint.a);

        AEMtx33 transform;
        AEMtx33Scale(&transform, width, height);
        
        AEMtx33 translate;
        AEMtx33Trans(&translate, x, y);
        
        AEMtx33 finalMtx;
        AEMtx33Concat(&finalMtx, &translate, &transform);
        
        AEGfxSetTransform(finalMtx.m);
        AEGfxMeshDraw(m_quadMesh, AE_GFX_MDM_TRIANGLES);
    }

    void Minimap::DrawQuadRotated(f32 x, f32 y, f32 width, f32 height, f32 rotationRad, const Engine::Color& color)
    {
        if (!m_quadMesh) return;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(color.r, color.g, color.b, color.a);
        AEGfxSetTransparency(color.a);

        AEMtx33 scale;
        AEMtx33Scale(&scale, width, height);
        
        AEMtx33 rotate;
        AEMtx33Rot(&rotate, rotationRad);
        
        AEMtx33 translate;
        AEMtx33Trans(&translate, x, y);
        
        AEMtx33 temp;
        AEMtx33Concat(&temp, &rotate, &scale);
        
        AEMtx33 finalMtx;
        AEMtx33Concat(&finalMtx, &translate, &temp);
        
        AEGfxSetTransform(finalMtx.m);
        AEGfxMeshDraw(m_quadMesh, AE_GFX_MDM_TRIANGLES);
    }

    void Minimap::DrawBorderFrame(const Rect& mapRect)
    {
        f32 left = mapRect.x;
        f32 right = mapRect.x + mapRect.width;
        f32 bottom = mapRect.y;
        f32 top = mapRect.y + mapRect.height;
        f32 t = borderThickness;

        if (m_borderSprite)
        {
            // Draw textured border frame (4 sides)
            // Left
            DrawQuadTextured(left - t * 0.5f, (top + bottom) * 0.5f, t, mapRect.height + t * 2.0f, m_borderSprite.get(), borderColor);
            // Right
            DrawQuadTextured(right + t * 0.5f, (top + bottom) * 0.5f, t, mapRect.height + t * 2.0f, m_borderSprite.get(), borderColor);
            // Top
            DrawQuadTextured((left + right) * 0.5f, top + t * 0.5f, mapRect.width, t, m_borderSprite.get(), borderColor);
            // Bottom
            DrawQuadTextured((left + right) * 0.5f, bottom - t * 0.5f, mapRect.width, t, m_borderSprite.get(), borderColor);
        }
        else
        {
            // Draw colored border frame (4 sides)
            // Left
            DrawQuad(left - t * 0.5f, (top + bottom) * 0.5f, t, mapRect.height + t * 2.0f, borderColor);
            // Right
            DrawQuad(right + t * 0.5f, (top + bottom) * 0.5f, t, mapRect.height + t * 2.0f, borderColor);
            // Top
            DrawQuad((left + right) * 0.5f, top + t * 0.5f, mapRect.width, t, borderColor);
            // Bottom
            DrawQuad((left + right) * 0.5f, bottom - t * 0.5f, mapRect.width, t, borderColor);
        }
    }

    void Minimap::DrawMaskBorders(const Rect& mapRect, const Engine::Color& bgColor)
    {
        // Draw 4 border quads around the minimap to cover any tile overflow
        // Border starts just outside the visible frame area
        f32 maskThickness = 50.0f;
        f32 frameOffset = borderThickness;
        
        f32 left = mapRect.x;
        f32 right = mapRect.x + mapRect.width;
        f32 bottom = mapRect.y;
        f32 top = mapRect.y + mapRect.height;

        // Mask extends from frame edge outward
        // Left mask
        DrawQuad(left - frameOffset - maskThickness * 0.5f, (top + bottom) * 0.5f, 
                 maskThickness, mapRect.height + frameOffset * 2.0f + maskThickness * 2.0f, bgColor);
        
        // Right mask
        DrawQuad(right + frameOffset + maskThickness * 0.5f, (top + bottom) * 0.5f, 
                 maskThickness, mapRect.height + frameOffset * 2.0f + maskThickness * 2.0f, bgColor);
        
        // Top mask
        DrawQuad((left + right) * 0.5f, top + frameOffset + maskThickness * 0.5f, 
                 mapRect.width + frameOffset * 2.0f, maskThickness, bgColor);
        
        // Bottom mask
        DrawQuad((left + right) * 0.5f, bottom - frameOffset - maskThickness * 0.5f, 
                 mapRect.width + frameOffset * 2.0f, maskThickness, bgColor);
    }

    void Minimap::Render()
    {
        if (!visible) return;
        if (!m_tileMap) return;

        Rect worldRect = GetWorldRect();
        f32 mapCenterX = worldRect.x + worldRect.width * 0.5f;
        f32 mapCenterY = worldRect.y + worldRect.height * 0.5f;
        f32 mapWidth = worldRect.width;
        f32 mapHeight = worldRect.height;

        // Draw background first
        DrawQuad(mapCenterX, mapCenterY, mapWidth, mapHeight, backgroundColor);

        // Get player position and rotation
        f32 playerTileX = 0.0f;
        f32 playerTileY = 0.0f;
        f32 playerRotation = 0.0f;

        if (m_playerTransform)
        {
            playerTileX = m_playerTransform->position.x;
            playerTileY = m_playerTransform->position.y;
            playerRotation = m_playerTransform->rotation;
        }

        // Convert player rotation to radians
        f32 rotRad = -(playerRotation + 90.0f) * Engine::Mathf::Deg2Rad;
        f32 cosR = Engine::Mathf::Cos(rotRad);
        f32 sinR = Engine::Mathf::Sin(rotRad);

        // Calculate tile size based on view radius
        int tilesVisible = viewRadius * 2 + 1;
        f32 tileSize = Engine::Mathf::Min(mapWidth, mapHeight) / static_cast<f32>(tilesVisible);

        // Draw tiles centered on player position with rotation
        int searchRadius = viewRadius + 3;
        
        for (int dy = -searchRadius; dy <= searchRadius; ++dy)
        {
            for (int dx = -searchRadius; dx <= searchRadius; ++dx)
            {
                // World tile coordinates (integer)
                int tileX = Engine::Mathf::FloorToInt(playerTileX) + dx;
                int tileY = Engine::Mathf::FloorToInt(playerTileY) + dy;

                // Determine tile color - treat out-of-bounds as walls
                Engine::Color tileColor = wallColor;
                
                if (tileX >= 0 && tileX < m_tileMap->width && 
                    tileY >= 0 && tileY < m_tileMap->height)
                {
                    Engine::TileType tile = m_tileMap->GetTile(tileX, tileY);
                    if (static_cast<int>(tile) == 0)
                    {
                        tileColor = floorColor;
                    }
                }

                // Calculate offset from player (sub-tile precision for smooth scrolling)
                f32 fracX = playerTileX - Engine::Mathf::Floor(playerTileX);
                f32 fracY = playerTileY - Engine::Mathf::Floor(playerTileY);
                
                f32 offsetX = (dx - fracX) * tileSize;
                f32 offsetY = -(dy - fracY) * tileSize;

                // Rotate the offset around the center
                f32 rotatedX = offsetX * cosR - offsetY * sinR;
                f32 rotatedY = offsetX * sinR + offsetY * cosR;

                f32 screenX = mapCenterX + rotatedX;
                f32 screenY = mapCenterY + rotatedY;

                // Draw tile with rotation
                DrawQuadRotated(screenX, screenY, tileSize, tileSize, rotRad, tileColor);
            }
        }

        // Draw player icon at center
        f32 playerSize = tileSize * 0.8f;
        if (m_playerSprite)
        {
            DrawQuadTextured(mapCenterX, mapCenterY, playerSize, playerSize, m_playerSprite.get(), playerColor);
        }
        else
        {
            DrawQuad(mapCenterX, mapCenterY, playerSize, playerSize, playerColor);
        }
        // Draw mask to cover tile overflow first
        Engine::Color maskColor(0.0f, 0.0f, 0.0f, 1.0f);
        DrawMaskBorders(worldRect, maskColor);

        // Draw border frame on top
        DrawBorderFrame(worldRect);
    }

} // namespace UI
