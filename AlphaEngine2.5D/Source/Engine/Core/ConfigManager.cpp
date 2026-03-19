/**
 * @file ConfigManager.cpp
 * @brief Implementation of ConfigManager file parsing using jsoncpp.
 */

#include "ConfigManager.hpp"
#include "Application.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Graphics/RendererSettings.hpp"
#include "Engine/Graphics/DoorRenderer.hpp"
#include "Engine/Physics/Physics2D.hpp"
#include "Engine/TileMap/Door.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include <fstream>

namespace Engine
{
    bool ConfigManager::LoadJson(const std::string& filepath, Json::Value& root)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            printf("[Warning] ConfigManager: Could not open file: %s\n", filepath.c_str());
            return false;
        }

        Json::CharReaderBuilder builder;
        JSONCPP_STRING errs;
        if (!parseFromStream(builder, file, &root, &errs))
        {
            printf("[Warning] ConfigManager: Failed to parse JSON file for %s: %s\n", filepath.c_str(), errs.c_str());
            return false;
        }
        
        return true;
    }

    void ConfigManager::InitializeAll()
    {
        Application::Initialize();      // application.json  → targetFrameRate
        Camera::Initialize();           // camera.json       → backgroundColor, fov, fog, ...
        Physics2D::Initialize();        // physics.json      → gravity, radii, thresholds
        RendererSettings::Initialize(); // rendering.json    → perpWallDistClamp, fog LUT, ...
        Door::Initialize();             // door.json         → audio distance, passable threshold
        DoorRenderer::Initialize();     // door.json         → fallback colors, fov
        SceneManager::Initialize();     // game.json         → fade durations
    }

} // namespace Engine
