#pragma once
/**
 * @file Texture2D.hpp
 * @brief Wrapper class for loading and managing PNG textures
 */

#include "AEEngine.h"
#include <string>

namespace UI
{
    /**
     * @class Texture2D
     * @brief RAII wrapper for AEGfxTexture (PNG file loading)
     * 
     * Handles texture loading and unloading automatically.
     * Use for UI image sprites.
     */
	 // TODO: [Caleb.A] Move this class to Engine/Graphics and use it for Sprite as well
    class Texture2D
    {
    private:
        AEGfxTexture* m_texture = nullptr;
        std::string m_filePath;

        // Singleton white texture
        static Texture2D* s_whiteTexture;

    public:
        /**
         * @brief Default constructor (no texture loaded)
         */
        Texture2D() = default;

        /**
         * @brief Construct and load texture from file
         * @param filePath Path to PNG file (e.g., "Assets/UI/button.png")
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
         * @brief Get a 1x1 white texture for solid color rendering
         * @return Pointer to the shared white texture (never null after first call)
         */
        static Texture2D* WhiteTexture();

        /**
         * @brief Initialize static resources (call during UIManager::Initialize)
         */
        static void InitializeStatics();

        /**
         * @brief Cleanup static resources (call during UIManager::Shutdown)
         */
        static void ShutdownStatics();
    };

} // namespace UI
