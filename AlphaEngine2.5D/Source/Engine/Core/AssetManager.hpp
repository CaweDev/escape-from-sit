#pragma once
/**
 * @file AssetManager.hpp
 * @brief Static asset management class for loading and caching Sprites and Textures
 */

#include "../Graphics/Sprite.hpp"
#include "../Graphics/Texture2D.hpp"
#include <string>
#include <unordered_map>

namespace Engine
{
    /**
     * @class AssetManager
     * @brief Static class for centralized asset loading and caching
     * 
     * Provides methods to load and cache Sprites and Textures.
     * Assets are cached by file path to avoid duplicate loading.
     */
    class AssetManager
    {
    private:
        static std::unordered_map<std::string, Sprite*> s_spriteCache;
        static std::unordered_map<std::string, Texture2D*> s_textureCache;

        // Prevent instantiation
        AssetManager() = delete;
        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;

    public:
        /**
         * @brief Get or load a Sprite from file
         * @param filepath Path to the image file (PNG)
         * @return Pointer to cached Sprite (owned by AssetManager)
         */
        static Sprite* GetSprite(const std::string& filepath);

        /**
         * @brief Get or load a Texture2D from file
         * @param filepath Path to the image file (PNG)
         * @return Pointer to cached Texture2D (owned by AssetManager)
         */
        static Texture2D* GetTexture(const std::string& filepath);

        /**
         * @brief Unload a specific Sprite from cache
         * @param filepath Path of the sprite to unload
         */
        static void UnloadSprite(const std::string& filepath);

        /**
         * @brief Unload a specific Texture from cache
         * @param filepath Path of the texture to unload
         */
        static void UnloadTexture(const std::string& filepath);

        /**
         * @brief Unload all cached assets
         */
        static void UnloadAll();

        /**
         * @brief Get number of cached sprites
         */
        static size_t GetSpriteCacheSize() { return s_spriteCache.size(); }

        /**
         * @brief Get number of cached textures
         */
        static size_t GetTextureCacheSize() { return s_textureCache.size(); }

        /**
         * @brief Check if a sprite is cached
         */
        static bool HasSprite(const std::string& filepath);

        /**
         * @brief Check if a texture is cached
         */
        static bool HasTexture(const std::string& filepath);
    };

} // namespace Engine
