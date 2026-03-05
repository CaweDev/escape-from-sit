/**
 * @file AssetManager.cpp
 * @brief Implementation of AssetManager static class
 */

#include "AssetManager.hpp"
#include <json/json.h>
#include <fstream>

namespace Engine
{
    // Helper to read JSON meta
    static bool LoadJsonMeta(const std::string& filepath, Json::Value& root)
    {
        std::ifstream file(filepath + ".meta");
        if (!file.is_open())
            return false;
            
        Json::CharReaderBuilder builder;
        JSONCPP_STRING errs;
        if (!parseFromStream(builder, file, &root, &errs))
        {
            printf("[Warning] AssetManager: Failed to parse meta file for %s: %s\n", filepath.c_str(), errs.c_str());
            return false;
        }
        return true;
    }

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
            
            Json::Value root;
            if (LoadJsonMeta(filepath, root))
            {
                if (root.isMember("nineSlice"))
                {
                    const auto& ns = root["nineSlice"];
                    NineSliceBorder border;
                    border.left   = ns.get("left", 0.0f).asFloat();
                    border.right  = ns.get("right", 0.0f).asFloat();
                    border.top    = ns.get("top", 0.0f).asFloat();
                    border.bottom = ns.get("bottom", 0.0f).asFloat();
                    texture->SetSliceBorder(border);
                }
            }
            
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

    SpriteSheet* AssetManager::GetSpriteSheet(const std::string& filepath)
    {
        auto it = s_spritesheetCache.find(filepath);
        if (it != s_spritesheetCache.end())
        {
            return it->second;
        }
        
        Json::Value root;
        int rows = 1;
        int cols = 1;
        
        bool hasMeta = LoadJsonMeta(filepath, root);
        if (hasMeta)
        {
             rows = root.get("rows", 1).asInt();
             cols = root.get("cols", 1).asInt();
        }
        else
        {
             printf("[Warning] AssetManager: Loading %s without explicit rows/cols and no .meta. Defaulting to 1x1 grid.\n", filepath.c_str());
        }
        
        SpriteSheet* sheet = new SpriteSheet();
        if (sheet->Load(filepath, rows, cols))
        {
            s_spritesheetCache[filepath] = sheet;
            
            if (hasMeta && root.isMember("animations") && root["animations"].isArray())
            {
                for (const auto& anim : root["animations"])
                {
                    std::string name = anim.get("name", "unknown").asString();
                    int startFrame = anim.get("startFrame", 0).asInt();
                    int frameCount = anim.get("frameCount", 1).asInt();
                    float fps = anim.get("fps", 10.0f).asFloat();
                    bool loop = anim.get("loop", true).asBool();
                    sheet->AddAnimation(name, startFrame, frameCount, fps, loop);
                }
            }
            
            return sheet;
        }

        delete sheet;
        return nullptr;
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
            
            Json::Value root;
            if (LoadJsonMeta(filepath, root) && root.isMember("animations") && root["animations"].isArray())
            {
                for (const auto& anim : root["animations"])
                {
                    std::string name = anim.get("name", "unknown").asString();
                    int startFrame = anim.get("startFrame", 0).asInt();
                    int frameCount = anim.get("frameCount", 1).asInt();
                    float fps = anim.get("fps", 10.0f).asFloat();
                    bool loop = anim.get("loop", true).asBool();
                    sheet->AddAnimation(name, startFrame, frameCount, fps, loop);
                }
            }
            
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
