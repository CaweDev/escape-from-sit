/**
 * @file NineSliceRenderer.cpp
 * @brief Implementation of 9-slice rendering utility
 */

#include "Engine/UI/Helpers/NineSliceRenderer.hpp"
#include "Engine/UI/Core/UIManager.hpp"

namespace UI
{
    void NineSliceRenderer::Draw(const Rect& worldRect,
                                  Engine::Texture2D* texture,
                                  const Engine::Color& color)
    {
        if (!texture || !texture->IsLoaded()) return;

        int texW = texture->GetPixelWidth();
        int texH = texture->GetPixelHeight();
        if (texW <= 0 || texH <= 0) return;

        const Engine::NineSliceBorder& border = texture->GetSliceBorder();

        // --- Convert pixel borders to UV-normalized values ---
        f32 uLeft   = border.left   / (f32)texW;
        f32 uRight  = border.right  / (f32)texW;
        f32 vTop    = border.top    / (f32)texH;
        f32 vBottom = border.bottom / (f32)texH;

        // --- Convert pixel borders to world-space sizes ---
        // Scale borders proportionally to how the texture maps onto the rect
        f32 worldBorderLeft   = border.left   * (worldRect.width / (f32)texW);
        f32 worldBorderRight  = border.right  * (worldRect.width / (f32)texW);
        f32 worldBorderTop    = border.top    * (worldRect.height / (f32)texH);
        f32 worldBorderBottom = border.bottom * (worldRect.height / (f32)texH);

        // Clamp borders if they exceed the rect size
        f32 totalHBorder = worldBorderLeft + worldBorderRight;
        f32 totalVBorder = worldBorderTop + worldBorderBottom;

        if (totalHBorder > worldRect.width)
        {
            f32 scale = worldRect.width / totalHBorder;
            worldBorderLeft *= scale;
            worldBorderRight *= scale;
        }
        if (totalVBorder > worldRect.height)
        {
            f32 scale = worldRect.height / totalVBorder;
            worldBorderTop *= scale;
            worldBorderBottom *= scale;
        }

        // --- Calculate position grid (4 x-positions, 4 y-positions) ---
        // World space: x increases right, y increases up
        f32 x0 = worldRect.x;                                      // Left edge
        f32 x1 = worldRect.x + worldBorderLeft;                    // Left border end
        f32 x2 = worldRect.x + worldRect.width - worldBorderRight; // Right border start
        f32 x3 = worldRect.x + worldRect.width;                    // Right edge

        f32 y0 = worldRect.y;                                       // Bottom edge
        f32 y1 = worldRect.y + worldBorderBottom;                   // Bottom border end
        f32 y2 = worldRect.y + worldRect.height - worldBorderTop;  // Top border start
        f32 y3 = worldRect.y + worldRect.height;                    // Top edge

        // --- Calculate UV grid ---
        // AEGfx UV convention: V=0 is top of texture, V=1 is bottom
        f32 u0 = 0.0f;
        f32 u1 = uLeft;
        f32 u2 = 1.0f - uRight;
        f32 u3 = 1.0f;

        f32 v0 = 1.0f;           // Bottom of texture (V=1)
        f32 v1 = 1.0f - vBottom; // Bottom border end in UV
        f32 v2 = vTop;           // Top border start in UV
        f32 v3 = 0.0f;           // Top of texture (V=0)

        // --- Build transient mesh with 9 quads (18 triangles) ---
        AEGfxMeshStart();

        // Bottom row: BL, BC, BR
        AddQuad(x0, y0, x1, y1, u0, v0, u1, v1);  // Bottom-Left
        AddQuad(x1, y0, x2, y1, u1, v0, u2, v1);  // Bottom-Center
        AddQuad(x2, y0, x3, y1, u2, v0, u3, v1);  // Bottom-Right

        // Middle row: ML, MC, MR
        AddQuad(x0, y1, x1, y2, u0, v1, u1, v2);  // Middle-Left
        AddQuad(x1, y1, x2, y2, u1, v1, u2, v2);  // Middle-Center (stretches both axes)
        AddQuad(x2, y1, x3, y2, u2, v1, u3, v2);  // Middle-Right

        // Top row: TL, TC, TR
        AddQuad(x0, y2, x1, y3, u0, v2, u1, v3);  // Top-Left
        AddQuad(x1, y2, x2, y3, u1, v2, u2, v3);  // Top-Center
        AddQuad(x2, y2, x3, y3, u2, v2, u3, v3);  // Top-Right

        AEGfxVertexList* mesh = AEGfxMeshEnd();
        if (!mesh) return;

        // --- Set render state ---
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(texture->GetNativeTexture(), 0, 0);
        AEGfxSetColorToMultiply(color.r, color.g, color.b, color.a);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(color.a);

        // Draw with identity transform — vertices are already in world coordinates
        AEMtx33 identity;
        AEMtx33Identity(&identity);
        AEGfxSetTransform(identity.m);
        AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);

        // Free the transient mesh
        AEGfxMeshFree(mesh);
    }

    void NineSliceRenderer::AddQuad(f32 x0, f32 y0, f32 x1, f32 y1,
                                     f32 u0, f32 v0, f32 u1, f32 v1)
    {
        // Skip degenerate quads (zero width or height)
        if (x1 <= x0 || y1 <= y0) return;

        // Triangle 1: bottom-left, top-left, top-right
        AEGfxTriAdd(
            x0, y0, 0xFFFFFFFF, u0, v0,  // bottom-left
            x0, y1, 0xFFFFFFFF, u0, v1,  // top-left
            x1, y1, 0xFFFFFFFF, u1, v1   // top-right
        );

        // Triangle 2: bottom-left, top-right, bottom-right
        AEGfxTriAdd(
            x0, y0, 0xFFFFFFFF, u0, v0,  // bottom-left
            x1, y1, 0xFFFFFFFF, u1, v1,  // top-right
            x1, y0, 0xFFFFFFFF, u1, v0   // bottom-right
        );
    }

} // namespace UI
