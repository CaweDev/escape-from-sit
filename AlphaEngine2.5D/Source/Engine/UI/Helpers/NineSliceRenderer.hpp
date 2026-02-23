#pragma once
/**
 * @file NineSliceRenderer.hpp
 * @brief Static utility for 9-slice (9-patch) UI rendering
 *
 * Draws a texture split into 9 regions so that corners stay fixed-size,
 * edges stretch along one axis, and the center fills the remaining area.
 */

#include "AEEngine.h"
#include "Engine/UI/Core/RectTransform.hpp"
#include "Engine/Graphics/Texture2D.hpp"
#include "Engine/Graphics/Color.hpp"

namespace UI
{
    /**
     * @class NineSliceRenderer
     * @brief Static utility class for drawing 9-sliced images
     *
     * Builds a transient 18-triangle mesh with correct per-patch UVs,
     * draws it in a single draw call, then frees it.
     *
     * Border widths are read from the texture's NineSliceBorder (set via
     * Texture2D::SetSliceBorder). The border is an intrinsic property of
     * the texture asset.
     *
     * 9-Slice layout:
     * @code
     *   +--------+-----------+--------+
     *   |  TL    |    TC     |   TR   |  <- border.top (fixed height)
     *   +--------+-----------+--------+
     *   |  ML    |    MC     |   MR   |  <- stretches vertically
     *   +--------+-----------+--------+
     *   |  BL    |    BC     |   BR   |  <- border.bottom (fixed height)
     *   +--------+-----------+--------+
     *     border    stretches   border
     *     .left                 .right
     * @endcode
     */
    class NineSliceRenderer
    {
    public:
        /**
         * @brief Draw a 9-sliced image using the texture's intrinsic border
         *
         * @param worldRect  World-space rectangle to fill
         * @param texture    Source texture (must be loaded, with valid pixel dimensions and border set)
         * @param color      Tint/multiply color
         */
        static void Draw(const Rect& worldRect,
                         Engine::Texture2D* texture,
                         const Engine::Color& color);

    private:
        /**
         * @brief Add two triangles (one quad) to the current open AEGfx mesh
         *
         * @param x0,y0  Bottom-left corner in world space
         * @param x1,y1  Top-right corner in world space
         * @param u0,v0  UV at bottom-left (note: AEGfx V=0 is top, V=1 is bottom)
         * @param u1,v1  UV at top-right
         */
        static void AddQuad(f32 x0, f32 y0, f32 x1, f32 y1,
                            f32 u0, f32 v0, f32 u1, f32 v1);
    };

} // namespace UI
