#pragma once
/**
 * @file Texture2D.hpp
 * @brief Wrapper class for loading and managing PNG textures
 */

#include "AEEngine.h"
#include <string>

namespace Engine
{
    /**
     * @struct NineSliceBorder
     * @brief Border widths in source-texture pixels for 9-slice rendering
     *
     * Defines how far inward from each edge the "fixed" corner/edge regions
     * extend. The center region stretches to fill the remaining area.
     */
    struct NineSliceBorder
    {
        f32 left   = 0;  ///< Pixels from the left edge
        f32 right  = 0;  ///< Pixels from the right edge
        f32 top    = 0;  ///< Pixels from the top edge
        f32 bottom = 0;  ///< Pixels from the bottom edge
    };
    /**
     * @class Texture2D
     * @brief RAII wrapper for AEGfxTexture (PNG file loading)
     * 
     * Handles texture loading and unloading automatically.
     */
    class Texture2D
    {
    private:
        AEGfxTexture* m_texture = nullptr;
        std::string m_filePath;
        int m_pixelWidth  = 0;
        int m_pixelHeight = 0;
        NineSliceBorder m_sliceBorder;  ///< 9-slice border widths (intrinsic to the asset)

        // Singleton white texture
        static Texture2D* s_whiteTexture;

    public:
        /**
         * @brief Default constructor (no texture loaded)
         */
        Texture2D() = default;

        /**
         * @brief Construct and load texture from file
         * @param filePath Path to PNG file
         */
        explicit Texture2D(const std::string& filePath);

        /**
         * @brief Destructor - automatically unloads texture
         */
        ~Texture2D();

        // Prevent copying (textures are resources)
        Texture2D(const Texture2D&) = delete;
        Texture2D& operator=(const Texture2D&) = delete;

        // Allow moving
        Texture2D(Texture2D&& other) noexcept;
        Texture2D& operator=(Texture2D&& other) noexcept;

        /**
         * @brief Load a texture from file
         * @param filePath Path to PNG file
         * @return True if loading succeeded
         */
        bool Load(const std::string& filePath);

        /**
         * @brief Unload the current texture
         */
        void Unload();

        /**
         * @brief Check if a texture is loaded
         */
        bool IsLoaded() const { return m_texture != nullptr; }

        /**
         * @brief Get the raw AEGfxTexture pointer
         * @return Native texture handle (may be null)
         */
        AEGfxTexture* GetNativeTexture() const { return m_texture; }

        /**
         * @brief Get the file path this texture was loaded from
         */
        const std::string& GetFilePath() const { return m_filePath; }

        /**
         * @brief Get the pixel width of the loaded texture
         * @return Width in pixels, or 0 if not loaded
         */
        int GetPixelWidth() const { return m_pixelWidth; }

        /**
         * @brief Get the pixel height of the loaded texture
         * @return Height in pixels, or 0 if not loaded
         */
        int GetPixelHeight() const { return m_pixelHeight; }

        /**
         * @brief Set the 9-slice border widths (in source-texture pixels)
         * @param border Border widths for each edge
         */
        void SetSliceBorder(const NineSliceBorder& border) { m_sliceBorder = border; }

        /**
         * @brief Get the 9-slice border widths
         * @return Current border widths
         */
        const NineSliceBorder& GetSliceBorder() const { return m_sliceBorder; }

        /**
         * @brief Get a 1x1 white texture for solid color rendering
         * @return Pointer to the shared white texture
         */
        static Texture2D* WhiteTexture();

        /**
         * @brief Initialize static resources
         */
        static void InitializeStatics();

        /**
         * @brief Cleanup static resources
         */
        static void ShutdownStatics();
    };

} // namespace Engine
