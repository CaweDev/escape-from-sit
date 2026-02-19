/**
 * @file AssetManager.cpp
 * @brief Implementation of AssetManager static class
 */

#include "AssetManager.hpp"

namespace Engine
{
    // Static member initialization
    std::unordered_map<std::string, Sprite*> AssetManager::s_spriteCache;
    std::unordered_map<std::string, SpriteSheet*> AssetManager::s_spritesheetCache;
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

        // Unload all spritesheets
        for (auto& pair : s_spritesheetCache)
        {
            delete pair.second;
        }
        s_spritesheetCache.clear();

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

    SpriteSheet* AssetManager::GetSpriteSheet(const std::string& filepath, int rows, int cols)
    {
        auto it = s_spritesheetCache.find(filepath);
        if (it != s_spritesheetCache.end())
        {
            return it->second;
        }

        SpriteSheet* sheet = new SpriteSheet();
        if (sheet->Load(filepath, rows, cols))
        {
            s_spritesheetCache[filepath] = sheet;
            return sheet;
        }

        delete sheet;
        return nullptr;
    }

    void AssetManager::UnloadSpriteSheet(const std::string& filepath)
    {
        auto it = s_spritesheetCache.find(filepath);
        if (it != s_spritesheetCache.end())
        {
            delete it->second;
            s_spritesheetCache.erase(it);
        }
    }

    bool AssetManager::HasSpriteSheet(const std::string& filepath)
    {
        return s_spritesheetCache.find(filepath) != s_spritesheetCache.end();
    }

} // namespace Engine
