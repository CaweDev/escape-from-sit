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
int   RendererSettings::raycastColumnStep = 10;
int   RendererSettings::maxTrianglesPerMesh = 8000;
float RendererSettings::tilemapFbScale = 0.2f;
float RendererSettings::visibilityCullThreshold = 0.01f;
float RendererSettings::wallOverlapPadding = 10.0f;

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

    raycastColumnStep = (int)config.raycastColumnStep;
    if (raycastColumnStep < 1) raycastColumnStep = 1;

    maxTrianglesPerMesh = (int)config.maxTrianglesPerMesh;
    if (maxTrianglesPerMesh < 2) maxTrianglesPerMesh = 2;

    tilemapFbScale = config.tilemapFbScale;
    if (tilemapFbScale < 0.05f) tilemapFbScale = 0.05f;
    if (tilemapFbScale > 1.0f) tilemapFbScale = 1.0f;

    visibilityCullThreshold = config.visibilityCullThreshold;
    if (visibilityCullThreshold < 0.0f) visibilityCullThreshold = 0.0f;
    if (visibilityCullThreshold > 1.0f) visibilityCullThreshold = 1.0f;

    wallOverlapPadding = config.wallOverlapPadding;
    if (wallOverlapPadding < 0.0f) wallOverlapPadding = 0.0f;
}

} // namespace Engine
