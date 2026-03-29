/* Start Header ************************************************************************/
/*!
\file		ZBuffer.hpp
\author 	
\par	
\date	March, 29, 2026
\brief		[AUTO] ZBuffer

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include <vector>
#include "AEEngine.h"

namespace Engine {

    /**
     * @brief Static utility class for managing a 1D depth buffer (Z-buffer).
     * Used for occlusion in 2.5D rendering.
     */
    class ZBuffer {
    public:
        /**
         * @brief Resizes the buffer to match the screen width.
         * @param width The screen width in pixels.
         */
        static void Resize(int width);

        /**
         * @brief Clears the buffer with a very large depth value.
         * @param defaultDepth The value to fill the buffer with (default: infinity).
         */
        static void Clear(float defaultDepth = 1e30f);

        /**
         * @brief Sets the depth for a specific screen column.
         * @param x The column index (0 to width-1).
         * @param depth The perpendicular distance to the wall.
         */
        static void SetDepth(int x, float depth);

        /**
         * @brief Gets the depth for a specific screen column.
         * @param x The column index.
         * @return The stored depth value.
         */
        static float GetDepth(int x);

        /**
         * @brief Returns the current width of the buffer.
         */
        static int GetWidth();

    private:
        static std::vector<float> buffer;
    };

} // namespace Engine
