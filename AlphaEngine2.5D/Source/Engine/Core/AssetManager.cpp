/**
 * @file AssetManager.cpp
 * @brief Implementation of AssetManager static class
 */

#include "AssetManager.hpp"

namespace Engine
{
    // Static member initialization
    std::unordered_map<std::string, Sprite*> AssetManager::s_spriteCache;
    std::unordered_map<std::string, Texture2D*> AssetManager::s_textureCache;

    Sprite* AssetManager::GetSprite(const std::string& filepath)
    {
        // Check cache first
        auto it = s_spriteCache.find(filepath);
        if (it != s_spriteCache.end())
        {
            return it->second;
        }

        // Load new sprite
        Sprite* sprite = new Sprite();
        if (sprite->Load(filepath))
        {
            s_spriteCache[filepath] = sprite;
            return sprite;
        }

        // Loading failed, cleanup and return nullptr
        delete sprite;
        return nullptr;
    }

    Texture2D* AssetManager::GetTexture(const std::string& filepath)
    {
        // Check cache first
        auto it = s_textureCache.find(filepath);
        if (it != s_textureCache.end())
        {
            return it->second;
        }

        // Load new texture
        Texture2D* texture = new Texture2D(filepath);
        if (texture->IsLoaded())
        {
            s_textureCache[filepath] = texture;
            return texture;
        }

        // Loading failed, cleanup and return nullptr
        delete texture;
        return nullptr;
    }

    void AssetManager::UnloadSprite(const std::string& filepath)
    {
        auto it = s_spriteCache.find(filepath);
        if (it != s_spriteCache.end())
        {
            delete it->second;
            s_spriteCache.erase(it);
        }
    }

    void AssetManager::UnloadTexture(const std::string& filepath)
    {
        auto it = s_textureCache.find(filepath);
        if (it != s_textureCache.end())
        {
            delete it->second;
            s_textureCache.erase(it);
        }
    }

    void AssetManager::UnloadAll()
    {
        // Unload all sprites
        for (auto& pair : s_spriteCache)
        {
            delete pair.second;
        }
        s_spriteCache.clear();

        // Unload all textures
        for (auto& pair : s_textureCache)
        {
            delete pair.second;
        }
        s_textureCache.clear();
    }

    bool AssetManager::HasSprite(const std::string& filepath)
    {
        return s_spriteCache.find(filepath) != s_spriteCache.end();
    }

    bool AssetManager::HasTexture(const std::string& filepath)
    {
        return s_textureCache.find(filepath) != s_textureCache.end();
    }

} // namespace Engine
