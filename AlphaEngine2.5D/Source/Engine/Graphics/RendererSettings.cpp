/**
 * @file RendererSettings.cpp
 * @brief Implementation of RendererSettings — loads rendering.json via ConfigManager.
 */
#include "RendererSettings.hpp"
#include <cstdio>

namespace Engine {

// Static member definitions — values match the JSON defaults.
float RendererSettings::perpWallDistClamp   = 0.1f;
float RendererSettings::fogLutDistScale     = 100.0f;
float RendererSettings::fogRebuildThreshold = 0.001f;
float RendererSettings::doorRayDirThreshold = 0.0001f;
float RendererSettings::doorZBufferBias     = 0.01f;
float RendererSettings::projectionNearClamp = 0.1f;

void RendererSettings::Initialize(const std::string& configPath)
{
    RenderingConfig config;
    if (!ConfigManager::ReadConfig(configPath, config))
    {
        std::printf("[Warning] RendererSettings: Failed to read config '%s'. Using built-in defaults.\n",
                    configPath.c_str());
    }

    perpWallDistClamp   = config.perpWallDistClamp;
    fogLutDistScale     = config.fogLutDistScale;
    fogRebuildThreshold = config.fogRebuildThreshold;
    doorRayDirThreshold = config.doorRayDirThreshold;
    doorZBufferBias     = config.doorZBufferBias;
    projectionNearClamp = config.projectionNearClamp;
}

} // namespace Engine
