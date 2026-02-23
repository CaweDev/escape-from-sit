#pragma once
/**
 * @file TextureUtils.hpp
 * @brief Static utility class for texture operations not provided by AEEngine
 *
 * Provides CPU-side pixel loading via Windows Imaging Component (WIC).
 * Extracted from TileMapRenderer to enable reuse across the engine.
 */

#include "AEEngine.h"
#include <string>
#include <vector>

namespace Engine
{
    /**
     * @struct CpuTexture
     * @brief CPU-side copy of a texture's pixel data, loaded via WIC
     *
     * Stored as tightly-packed RGBA (4 bytes per pixel), row-major from top-left.
     */
    struct CpuTexture
    {
        std::vector<u8> pixels;  ///< RGBA pixel data, width * height * 4 bytes.
        int width  = 0;          ///< Texture width in pixels.
        int height = 0;          ///< Texture height in pixels.
    };

    /**
     * @class TextureUtils
     * @brief Static utility class for texture operations
     */
    class TextureUtils
    {
    public:
        /**
         * @brief Loads a PNG from disk into a CpuTexture using Windows Imaging Component (WIC)
         *
         * The result is a tightly-packed RGBA8 buffer. Initialises COM for the
         * calling thread if needed.
         *
         * @param path  File path to the PNG image (UTF-8).
         * @param out   Output CpuTexture to populate.
         * @return      True if loading succeeded; false on any WIC failure.
         */
        static bool LoadCpuPixels(const std::string& path, CpuTexture& out);
    };

} // namespace Engine
