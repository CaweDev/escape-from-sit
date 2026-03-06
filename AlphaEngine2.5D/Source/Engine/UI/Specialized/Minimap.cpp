/**
 * @file Minimap.cpp
 * @brief Implementation of the Minimap UI element
 */

#include "Engine/UI/Specialized/Minimap.hpp"
#include "AEEngine.h"

namespace UI
{
    Minimap::Minimap()
        : m_wallColor(0.4f, 0.4f, 0.5f, 1.0f)
        , m_floorColor(0.2f, 0.2f, 0.25f, 1.0f)
        , m_doorColor(0.9f, 0.7f, 0.2f, 1.0f)  // Orange/yellow for doors
        , m_playerColor(0.0f, 0.8f, 0.2f, 1.0f)
        , m_backgroundColor(0.1f, 0.1f, 0.12f, 0.85f)
        , m_borderColor(0.3f, 0.3f, 0.35f, 1.0f)
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
    void Minimap::AddEnemyTransform(Engine::Transform* transform)
    {
        if (!transform) return;
        m_enemyTransforms.push_back(transform);
    }
    void Minimap::AddCCTVTransform(Engine::Transform* transform)
    {
        if (!transform) return;
        m_cctvTransforms.push_back(transform);
    }
    void Minimap::ClearCCTVs() { 
        m_cctvTransforms.clear();
        m_cctvCones.clear();
    }
    void Minimap::SetCCTVSprites(Engine::Sprite* sprite) { m_cctvSprite = sprite; }
    
    void Minimap::AddTerminalIcon(const Engine::Vector2& worldPos, Engine::Sprite* sprite)
    {
        m_terminalIcons.push_back({ worldPos, sprite });
    }
    
    void Minimap::UpdateCCTVCone(Engine::Transform* transform, float coneWidth, float maxRange, bool isDetecting)
    {
        if (!transform) return;
        
        // Find existing entry or add new one
        for (auto& cone : m_cctvCones)
        {
            if (cone.transform == transform)
            {
                cone.coneWidth = coneWidth;
                cone.maxRange = maxRange;
                cone.isDetecting = isDetecting;
                return;
            }
        }
        
        // Not found, add new
        m_cctvCones.push_back({ transform, coneWidth, maxRange, isDetecting });
    }
    /**
     * @brief Generates a clipped vertex list for a quad using the Sutherland-Hodgman algorithm.
     * 
     * THE ALGORITHM CRASH COURSE:
     * Sutherland-Hodgman is a pipeline-based clipping algorithm used to clip a subject 
     * polygon (our tile) against a convex clipping polygon (our UI bounding box).
     * 
     * HOW IT WORKS:
     * 1. The algorithm processes the subject polygon against each edge of the clipping region sequentially.
     * 2. For each edge, it iterates through the current vertices as "pairs" (v1 to v2).
     * 3. It applies a state-machine logic based on whether the start/end points of an edge 
     *    are "inside" or "outside" the clipping boundary.
     * 4. This results in a new polygon that is perfectly trimmed to the visible area.
     * 
     * WHY WE USE IT HERE:
     * Since tiles are rotated to match player heading, simple AABB (Axis-Aligned Bounding Box) 
     * checks aren't enough. Sutherland-Hodgman allows us to render perfectly trimmed, 
     * rotated tiles at the edge of the minimap without using expensive GPU masking or 
     * stencil buffers.
     */
    std::vector<MinimapVertex> Minimap::GetClippedVertices(f32 x, f32 y, f32 width, f32 height, f32 rotationRad, const Engine::Color& color)
    {
        // Define the clipping boundaries derived from the UI's world-space Rect.
        Rect worldRect = GetWorldRect();
        f32 clipL = worldRect.x;
        f32 clipR = worldRect.x + worldRect.width;
        f32 clipB = worldRect.y;
        f32 clipT = worldRect.y + worldRect.height;

        u32 uColor = color.ToU32(); // Pack color once for efficiency

        // --- PHASE 1: VERTEX GENERATION ---
        // We start by calculating the 4 corners of the quad in its "rotated" position.
        f32 cosR = Engine::Mathf::Cos(rotationRad);
        f32 sinR = Engine::Mathf::Sin(rotationRad);
        f32 hw = width * 0.5f;
        f32 hh = height * 0.5f;

        // Lambda to transform local tile coordinates to the UI's coordinate system.
        auto GetWorldPos = [&](f32 lx, f32 ly) -> AEVec2 {
            AEVec2 p;
            p.x = x + (lx * cosR - ly * sinR);
            p.y = y + (lx * sinR + ly * cosR);
            return p;
        };

        // Initial vertex list (CCW order: Bottom-Left, Bottom-Right, Top-Right, Top-Left)
        std::vector<MinimapVertex> vertices = {
            { GetWorldPos(-hw, -hh), uColor, {0.0f, 1.0f} },
            { GetWorldPos( hw, -hh), uColor, {1.0f, 1.0f} },
            { GetWorldPos( hw,  hh), uColor, {1.0f, 0.0f} },
            { GetWorldPos(-hw,  hh), uColor, {0.0f, 0.0f} }
        };

        // --- PHASE 2: SEQUENTIAL CLIPPING ---
        // The 'Clip' lambda handles one boundary at a time (Left, Right, Bottom, Top).
        auto Clip = [&](const std::vector<MinimapVertex>& verts, int edge) -> std::vector<MinimapVertex> {
            std::vector<MinimapVertex> output;
            if (verts.empty()) return output;

            for (size_t i = 0; i < verts.size(); ++i) {
                const MinimapVertex& v1 = verts[i];                  // Current vertex
                const MinimapVertex& v2 = verts[(i + 1) % verts.size()]; // Next vertex in cycle

                // INSIDE/OUTSIDE TEST:
                // Determines if a point lies on the "valid" side of the clipping line.
                auto IsInside = [&](const MinimapVertex& v) -> bool {
                    switch (edge) {
                        case 0: return v.pos.x >= clipL; // Inside if to the right of left edge
                        case 1: return v.pos.x <= clipR; // Inside if to the left of right edge
                        case 2: return v.pos.y >= clipB; // Inside if above bottom edge
                        case 3: return v.pos.y <= clipT; // Inside if below top edge
                    }
                    return true;
                };

                // INTERSECTION CALCULATION:
                // Finds the exact point where the polygon edge crosses the clipping boundary.
                auto Intersect = [&](const MinimapVertex& a, const MinimapVertex& b) -> MinimapVertex {
                    f32 t = 0.0f; // Interpolation factor [0, 1]
                    switch (edge) {
                        case 0: t = (clipL - a.pos.x) / (b.pos.x - a.pos.x); break;
                        case 1: t = (clipR - a.pos.x) / (b.pos.x - a.pos.x); break;
                        case 2: t = (clipB - a.pos.y) / (b.pos.y - a.pos.y); break;
                        case 3: t = (clipT - a.pos.y) / (b.pos.y - a.pos.y); break;
                    }
                    
                    MinimapVertex res;
                    // Linear interpolation (Lerp) of position and UVs
                    res.pos.x = a.pos.x + t * (b.pos.x - a.pos.x);
                    res.pos.y = a.pos.y + t * (b.pos.y - a.pos.y);
                    res.uv.x = a.uv.x + t * (b.uv.x - a.uv.x);
                    res.uv.y = a.uv.y + t * (b.uv.y - a.uv.y);
                    // Also interpolate color to prevent harsh color snaps at edges
                    res.color = AEGfxColInterp(a.color, b.color, t);
                    return res;
                };

                bool in1 = IsInside(v1);
                bool in2 = IsInside(v2);

                // --- SUTHERLAND-HODGMAN 4-CASE LOGIC ---
                if (in1 && in2) {
                    // CASE 1: Both points inside. Keep the endpoint.
                    output.push_back(v2);
                } else if (in1 && !in2) {
                    // CASE 2: Exiting. Store the intersection point where it crossed out.
                    output.push_back(Intersect(v1, v2));
                } else if (!in1 && in2) {
                    // CASE 3: Entering. Store the intersection point AND the endpoint.
                    output.push_back(Intersect(v1, v2));
                    output.push_back(v2);
                }
                // CASE 4: Both outside. Discard both (nothing added to output).
            }
            return output;
        };

        // Process the vertices through the pipeline for each of the 4 clipping planes.
        for (int i = 0; i < 4; ++i) {
            vertices = Clip(vertices, i);
        }

        return vertices; // Flattened/Clipped vertex list ready for triangulation
    }

    void Minimap::DrawQuadClipped(f32 x, f32 y, f32 width, f32 height, f32 rotationRad, const Engine::Color& color, Engine::Sprite* sprite)
    {
        std::vector<MinimapVertex> vertices = GetClippedVertices(x, y, width, height, rotationRad, color);
        if (vertices.size() < 3) return;

        AEGfxSetRenderMode(sprite ? AE_GFX_RM_TEXTURE : AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        if (sprite) AEGfxTextureSet(sprite->GetTexture(), 0.0f, 0.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetTransparency(color.a);
        
        AEMtx33 identity;
        AEMtx33Identity(&identity);
        AEGfxSetTransform(identity.m);

        AEGfxMeshStart();
        for (size_t i = 1; i < vertices.size() - 1; ++i) {
            AEGfxTriAdd(
                vertices[0].pos.x, vertices[0].pos.y, vertices[0].color, vertices[0].uv.x, vertices[0].uv.y,
                vertices[i].pos.x, vertices[i].pos.y, vertices[i].color, vertices[i].uv.x, vertices[i].uv.y,
                vertices[i+1].pos.x, vertices[i+1].pos.y, vertices[i+1].color, vertices[i+1].uv.x, vertices[i+1].uv.y
            );
        }
        AEGfxVertexList* mesh = AEGfxMeshEnd();
        AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
        AEGfxMeshFree(mesh);
    }

    void Minimap::DrawBorderFrame(const Rect& mapRect)
    {
        f32 left = mapRect.x;
        f32 right = mapRect.x + mapRect.width;
        f32 bottom = mapRect.y;
        f32 top = mapRect.y + mapRect.height;
        f32 t = m_borderThickness;

        // Use a dummy world rect that is large enough to not clip the border
        // Actually, let's just use DrawQuadClipped with a very large rect? 
        // No, let's just implement a simple non-clipped DrawQuadInternal for decorative elements.
        auto DrawQuadInternal = [&](f32 x, f32 y, f32 width, f32 height, const Engine::Color& color, Engine::Sprite* sprite = nullptr) {
            AEGfxSetRenderMode(sprite ? AE_GFX_RM_TEXTURE : AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            if (sprite) AEGfxTextureSet(sprite->GetTexture(), 0.0f, 0.0f);
            AEGfxSetColorToMultiply(color.r, color.g, color.b, color.a);
            AEGfxSetTransparency(color.a);

            AEMtx33 scale, translate, finalMtx;
            AEMtx33Scale(&scale, width, height);
            AEMtx33Trans(&translate, x, y);
            AEMtx33Concat(&finalMtx, &translate, &scale);
            AEGfxSetTransform(finalMtx.m);
            AEGfxMeshDraw(m_quadMesh, AE_GFX_MDM_TRIANGLES);
        };

        // Draw border frame (4 sides)
        Engine::Sprite* bSprite = m_borderSprite ? m_borderSprite.get() : nullptr;
        // Left
        DrawQuadInternal(left - t * 0.5f, (top + bottom) * 0.5f, t, mapRect.height + t * 2.0f, m_borderColor, bSprite);
        // Right
        DrawQuadInternal(right + t * 0.5f, (top + bottom) * 0.5f, t, mapRect.height + t * 2.0f, m_borderColor, bSprite);
        // Top
        DrawQuadInternal((left + right) * 0.5f, top + t * 0.5f, mapRect.width, t, m_borderColor, bSprite);
        // Bottom
        DrawQuadInternal((left + right) * 0.5f, bottom - t * 0.5f, mapRect.width, t, m_borderColor, bSprite);
    }

    void Minimap::RenderCCTVCones(float mapCenterX, float mapCenterY, float tileSize,
                                   float playerTileX, float playerTileY, float playerRotation)
    {
        if (m_cctvCones.empty()) return;
        
        float rad = playerRotation * Engine::Mathf::Deg2Rad;
        float c = Engine::Mathf::Cos(rad);
        float s = Engine::Mathf::Sin(rad);
        
        // Get minimap bounds for culling
        Rect worldRect = GetWorldRect();
        float minimapLeft = worldRect.x;
        float minimapRight = worldRect.x + worldRect.width;
        float minimapBottom = worldRect.y;
        float minimapTop = worldRect.y + worldRect.height;
        
        // Batch all circles into one mesh
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        
        AEMtx33 identity;
        AEMtx33Identity(&identity);
        AEGfxSetTransform(identity.m);
        
        AEGfxMeshStart();
        
        bool hasGeometry = false;
        
        for (const auto& cone : m_cctvCones)
        {
            if (!cone.transform) continue;
            
            // World space position
            float cctvX = cone.transform->position.x;
            float cctvY = cone.transform->position.y;
            
            // Convert to minimap space
            float dxW = cctvX - playerTileX;
            float dyW = cctvY - playerTileY;
            float offX = dxW * tileSize;
            float offY = dyW * tileSize;
            float rotatedX = offX * s - offY * c;
            float rotatedY = offX * c + offY * s;
            
            float screenX = mapCenterX + rotatedX;
            float screenY = mapCenterY + rotatedY;
            
            // Draw circle representing detection range
            float radius = cone.maxRange * tileSize;
            
            // CULLING: Skip circles that are completely outside minimap bounds
            // Check if circle bounding box intersects with minimap
            if (screenX + radius < minimapLeft ||   // Circle completely to the left
                screenX - radius > minimapRight ||  // Circle completely to the right
                screenY + radius < minimapBottom || // Circle completely below
                screenY - radius > minimapTop)      // Circle completely above
            {
                continue; // Skip this CCTV, it's not visible
            }
            
            // Draw filled circle as triangle fan
            int segments = 20; // More segments = smoother circle
            Engine::Color color = cone.isDetecting ? m_cctvAlertColor : m_cctvNormalColor;
            u32 colorU32 = color.ToU32();
            
            for (int i = 0; i < segments; ++i)
            {
                float angle1 = (float)i / segments * 2.0f * 3.14159265f;
                float angle2 = (float)(i + 1) / segments * 2.0f * 3.14159265f;
                
                float x1 = screenX + Engine::Mathf::Cos(angle1) * radius;
                float y1 = screenY + Engine::Mathf::Sin(angle1) * radius;
                float x2 = screenX + Engine::Mathf::Cos(angle2) * radius;
                float y2 = screenY + Engine::Mathf::Sin(angle2) * radius;
                
                // Triangle: center -> edge1 -> edge2
                AEGfxTriAdd(
                    screenX, screenY, colorU32, 0, 0,      // Center
                    x1, y1, colorU32, 0, 0,                // Edge 1
                    x2, y2, colorU32, 0, 0                 // Edge 2
                );
            }
            
            hasGeometry = true;
        }
        
        // Draw all circles in one call (only if we added any geometry)
        AEGfxVertexList* mesh = AEGfxMeshEnd();
        if (mesh && hasGeometry) {
            AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
            AEGfxMeshFree(mesh);
        }
        else if (mesh) {
            // Free empty mesh
            AEGfxMeshFree(mesh);
        }
    }

    void Minimap::Render()
    {
        if (!visible) return;
        if (!m_tileMap) return;

        // Get the pixel dimensions and center of the minimap UI element
        Rect worldRect = GetWorldRect();
        f32 mapCenterX = worldRect.x + worldRect.width * 0.5f;
        f32 mapCenterY = worldRect.y + worldRect.height * 0.5f;
        f32 mapWidth = worldRect.width;
        f32 mapHeight = worldRect.height;

        // --- Step 1: Draw background panel ---
        DrawQuadClipped(mapCenterX, mapCenterY, mapWidth, mapHeight, 0.0f, m_backgroundColor);

        // Get current player state
        f32 playerTileX = 0.0f;
        f32 playerTileY = 0.0f;
        f32 playerRotation = 0.0f;

        if (m_playerTransform)
        {
            playerTileX = m_playerTransform->position.x;
            playerTileY = m_playerTransform->position.y;
            playerRotation = m_playerTransform->rotation;
        }

        // --- Step 2: Set up coordinate space for tile rotation ---
        // Project convention: +X is East, +Y is South. Turning Right increases playerRotation (Clockwise).
        // To keep the HUD "Forward-Up" without horizontal flipping:
        // 1. World East (+X) must map to HUD Up (+Y) when playerRotation is 0.
        // 2. World South (+Y) must map to HUD Right (+X) when playerRotation is 0.
        // 3. We then apply the player's rotation to the world sampling.
        f32 rad = playerRotation * Engine::Mathf::Deg2Rad;
        f32 c = Engine::Mathf::Cos(rad);
        f32 s = Engine::Mathf::Sin(rad);

        // Rotation for the individual tile quads to stay aligned with the mapping.
        // We MUST convert to radians as Mathf::Cos/Sin expect radians.
        // The 90 degree offset accounts for the fact that we map World +X to HUD +Y.
        f32 renderRotRad = (90.0f - playerRotation) * Engine::Mathf::Deg2Rad;

        // Calculate how large each tile should be to fit the requested view radius
        f32 tileSize = m_tilePixelSize;
        if (m_centerOnPlayer)
        {
            int tilesVisible = m_viewRadius * 2 + 1;
            tileSize = Engine::Mathf::Min(mapWidth, mapHeight) / static_cast<f32>(tilesVisible);
        }

        // --- Step 3: Batch render nearby tiles ---
        int searchRadius = static_cast<int>(m_viewRadius * 1.5f); 
        
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetTransparency(1.0f);
        
        AEMtx33 identity;
        AEMtx33Identity(&identity);
        AEGfxSetTransform(identity.m);

        AEGfxMeshStart();
        bool hasGeometry = false;

        for (int dy = -searchRadius; dy <= searchRadius; ++dy)
        {
            for (int dx = -searchRadius; dx <= searchRadius; ++dx)
            {
                int tileX = Engine::Mathf::FloorToInt(playerTileX) + dx;
                int tileY = Engine::Mathf::FloorToInt(playerTileY) + dy;

                Engine::Color tileColor = m_wallColor;
                if (tileX >= 0 && tileX < m_tileMap->width && tileY >= 0 && tileY < m_tileMap->height)
                {
                    int tileValue = static_cast<int>(m_tileMap->GetTile(tileX, tileY));
                    if (tileValue == 0) {
                        tileColor = m_floorColor;
                    } else if (tileValue >= 10 && tileValue <= 13) {
                        // Doors (10-13) show as orange/yellow
                        tileColor = m_doorColor;
                    }
                }

                f32 fracX = playerTileX - Engine::Mathf::Floor(playerTileX);
                f32 fracY = playerTileY - Engine::Mathf::Floor(playerTileY);
                
                f32 offX = (dx - fracX + 0.5f) * tileSize;
                f32 offY = (dy - fracY + 0.5f) * tileSize;

                // CORRECTED MAPPING:
                // HUD Y (Forward) = World X * cos(A) + World Y * sin(A)
                // HUD X (Right) = World Y * cos(A) - World X * sin(A)
                f32 rotatedX = offX * s - offY * c;
                f32 rotatedY = offX * c + offY * s;

                // Run clipping for this specific tile.
                std::vector<MinimapVertex> verts = GetClippedVertices(mapCenterX + rotatedX, mapCenterY + rotatedY, tileSize, tileSize, renderRotRad, tileColor);
                if (verts.size() >= 3) {
                    hasGeometry = true;
                    // Add vertices to the frame mesh as a triangle fan
                    for (size_t i = 1; i < verts.size() - 1; ++i) {
                        AEGfxTriAdd(
                            verts[0].pos.x, verts[0].pos.y, verts[0].color, verts[0].uv.x, verts[0].uv.y,
                            verts[i].pos.x, verts[i].pos.y, verts[i].color, verts[i].uv.x, verts[i].uv.y,
                            verts[i+1].pos.x, verts[i+1].pos.y, verts[i+1].color, verts[i+1].uv.x, verts[i+1].uv.y
                        );
                    }
                }
            }
        }

        // Finalize and draw the batched tiles
        if (hasGeometry) {
            AEGfxVertexList* mesh = AEGfxMeshEnd();
            AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
            AEGfxMeshFree(mesh);
        } else {
            AEGfxMeshEnd(); 
        }

        // --- Step 4: Overlay the player icon ---
        // The player is always at the center of the minimap when m_centerOnPlayer is true
        f32 playerSize = tileSize * 0.8f;
        DrawQuadClipped(mapCenterX, mapCenterY, playerSize, playerSize, 0.0f, m_playerColor, m_playerSprite.get());

        // --- Step 4.5: Render CCTV vision circles (DISABLED - needs proper clipping) ---
        // TODO: Implement proper circle clipping to minimap bounds
        // RenderCCTVCones(mapCenterX, mapCenterY, tileSize, playerTileX, playerTileY, playerRotation);

        f32 cctvSize = tileSize * 0.55f;

        for (auto* cctvTf : m_cctvTransforms)
        {
            if (!cctvTf) continue;

            f32 dxW = cctvTf->position.x - playerTileX;
            f32 dyW = cctvTf->position.y - playerTileY;

            f32 offX = dxW * tileSize;
            f32 offY = dyW * tileSize;

            f32 rotatedX = offX * s - offY * c;
            f32 rotatedY = offX * c + offY * s;

            DrawQuadClipped(mapCenterX + rotatedX,
                mapCenterY + rotatedY,
                cctvSize,
                cctvSize,
                0.0f,
                m_cctvColor,
                m_cctvSprite);
        }

        f32 enemySize = tileSize * 0.6f;
        for (auto* enemyTf : m_enemyTransforms)
        {
            if (!enemyTf) continue;

            f32 dxW = enemyTf->position.x - playerTileX;
            f32 dyW = enemyTf->position.y - playerTileY;

            f32 offX = dxW * tileSize;
            f32 offY = dyW * tileSize;

            f32 rotatedX = offX * s - offY * c;
            f32 rotatedY = offX * c + offY * s;

            DrawQuadClipped(mapCenterX + rotatedX,
                mapCenterY + rotatedY,
                enemySize,
                enemySize,
                0.0f,
                m_enemyColor);
        }

        // --- Step 6: Render Terminal Icons ---
        f32 terminalSize = tileSize * 0.7f;
        for (const auto& terminal : m_terminalIcons)
        {
            f32 dxW = terminal.worldPos.x - playerTileX;
            f32 dyW = terminal.worldPos.y - playerTileY;

            f32 offX = dxW * tileSize;
            f32 offY = dyW * tileSize;

            f32 rotatedX = offX * s - offY * c;
            f32 rotatedY = offX * c + offY * s;

            DrawQuadClipped(mapCenterX + rotatedX,
                mapCenterY + rotatedY,
                terminalSize,
                terminalSize,
                0.0f,
                Engine::Color(1.0f, 1.0f, 0.0f, 1.0f), // Yellow tint for terminals
                terminal.sprite);
        }
        
        // --- Step 6.5: Render Path Waypoints (GTA-style line) ---
        RenderPathWaypoints(mapCenterX, mapCenterY, tileSize, playerTileX, playerTileY);

        // --- Step 7: Draw the decorative frame ---
        DrawBorderFrame(worldRect);

        // Render children
        UIElement::Render();
    }
    
    void Minimap::RenderPathWaypoints(float mapCenterX, float mapCenterY, float tileSize,
                                      float playerTileX, float playerTileY)
    {
        if (m_pathWaypoints.empty()) return;
        
        // Calculate player rotation for coordinate transformation
        float playerRotation = m_playerTransform ? m_playerTransform->rotation : 0.0f;
        float rad = playerRotation * Engine::Mathf::Deg2Rad;
        float c = Engine::Mathf::Cos(rad);
        float s = Engine::Mathf::Sin(rad);
        
        // Setup rendering
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        
        AEMtx33 identity;
        AEMtx33Identity(&identity);
        AEGfxSetTransform(identity.m);
        
        // Draw path as connected line segments
        AEGfxMeshStart();
        
        Engine::Color pathColor(1.0f, 1.0f, 0.0f, 0.8f); // Bright yellow with slight transparency
        u32 colorU32 = pathColor.ToU32();
        float lineWidth = tileSize * 0.25f; // Thick line, easy to see
        
        for (size_t i = 0; i < m_pathWaypoints.size(); ++i)
        {
            // Current waypoint
            const Engine::Vector2& wp = m_pathWaypoints[i];
            
            // Convert to minimap space
            float dxW = wp.x - playerTileX;
            float dyW = wp.y - playerTileY;
            float offX = dxW * tileSize;
            float offY = dyW * tileSize;
            float rotatedX = offX * s - offY * c;
            float rotatedY = offX * c + offY * s;
            float screenX = mapCenterX + rotatedX;
            float screenY = mapCenterY + rotatedY;
            
            // Draw waypoint marker (small square)
            float markerSize = tileSize * 0.4f;
            float halfMarker = markerSize * 0.5f;
            
            // Square waypoint marker
            AEGfxTriAdd(
                screenX - halfMarker, screenY - halfMarker, colorU32, 0, 0,
                screenX + halfMarker, screenY - halfMarker, colorU32, 0, 0,
                screenX - halfMarker, screenY + halfMarker, colorU32, 0, 0
            );
            AEGfxTriAdd(
                screenX + halfMarker, screenY - halfMarker, colorU32, 0, 0,
                screenX + halfMarker, screenY + halfMarker, colorU32, 0, 0,
                screenX - halfMarker, screenY + halfMarker, colorU32, 0, 0
            );
            
            // Draw line to next waypoint
            if (i < m_pathWaypoints.size() - 1)
            {
                const Engine::Vector2& nextWp = m_pathWaypoints[i + 1];
                
                // Convert next waypoint to minimap space
                float dxW2 = nextWp.x - playerTileX;
                float dyW2 = nextWp.y - playerTileY;
                float offX2 = dxW2 * tileSize;
                float offY2 = dyW2 * tileSize;
                float rotatedX2 = offX2 * s - offY2 * c;
                float rotatedY2 = offX2 * c + offY2 * s;
                float screenX2 = mapCenterX + rotatedX2;
                float screenY2 = mapCenterY + rotatedY2;
                
                // Draw thick line as a rectangle
                float dx = screenX2 - screenX;
                float dy = screenY2 - screenY;
                float length = Engine::Mathf::Sqrt(dx * dx + dy * dy);
                if (length > 0.01f)
                {
                    float nx = -dy / length;  // Perpendicular (normal)
                    float ny = dx / length;
                    
                    float halfWidth = lineWidth * 0.5f;
                    
                    // Line as quad (two triangles)
                    float x1 = screenX + nx * halfWidth;
                    float y1 = screenY + ny * halfWidth;
                    float x2 = screenX - nx * halfWidth;
                    float y2 = screenY - ny * halfWidth;
                    float x3 = screenX2 + nx * halfWidth;
                    float y3 = screenY2 + ny * halfWidth;
                    float x4 = screenX2 - nx * halfWidth;
                    float y4 = screenY2 - ny * halfWidth;
                    
                    AEGfxTriAdd(x1, y1, colorU32, 0, 0, x2, y2, colorU32, 0, 0, x3, y3, colorU32, 0, 0);
                    AEGfxTriAdd(x2, y2, colorU32, 0, 0, x4, y4, colorU32, 0, 0, x3, y3, colorU32, 0, 0);
                }
            }
        }
        
        AEGfxVertexList* mesh = AEGfxMeshEnd();
        if (mesh) {
            AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
            AEGfxMeshFree(mesh);
        }
    }

} // namespace UI
