#pragma once
/**
 * @file RendererSettings.hpp
 * @brief Shared rendering configuration loaded from Assets/Config/Engine/rendering.json.
 *
 * Exposes tunable rendering constants (near-plane clamps, fog LUT precision, door
 * intersection thresholds) as static members so that TileMapRenderer, DoorRenderer,
 * and ProjectionHelper all read from one authoritative source.
 *
 * Call RendererSettings::Initialize() once at startup (before any renderer is used).
 * If not called, sensible defaults matching the shipped values are used.
 *
 * Example:
 * \code
 * // In scene/game init:
 * Engine::RendererSettings::Initialize("Assets/Config/Engine/rendering.json");
 * \endcode
 */

#include <string>
#include <json/json.h>
#include "Engine/Core/ConfigManager.hpp"

namespace Engine {

/**
 * @brief IConfig implementation that deserialises rendering.json.
 */
struct RenderingConfig : public IConfig
{
    float perpWallDistClamp   = 0.1f;    ///< Minimum perpendicular wall distance to prevent divide-by-zero.
    float fogLutDistScale     = 100.0f;  ///< LUT entries per world unit (precision = 1/fogLutDistScale).
    float fogRebuildThreshold = 0.001f;  ///< Min fog-distance change that triggers a LUT rebuild.
    float doorRayDirThreshold = 0.0001f; ///< Minimum ray-direction component to be treated as non-parallel.
    float doorZBufferBias     = 0.01f;   ///< Depth bias added when comparing against the Z-buffer.
    float projectionNearClamp = 0.1f;    ///< Minimum 2D distance / perp-distance in ProjectionHelper.

    float raycastColumnStep = 10.0f;         ///< Horizontal ray step in pixels for TileMapRenderer.
    float maxTrianglesPerMesh = 8000.0f;     ///< Triangle flush threshold for TileMapRenderer batches.
    float tilemapFbScale = 0.2f;             ///< TileMapRenderer floor/ceiling framebuffer scale.
    float visibilityCullThreshold = 0.01f;   ///< Visibility threshold used to cull heavily fogged rows/columns.
    float wallOverlapPadding = 10.0f;        ///< Pixel overlap to avoid tiny gaps between walls and floor/ceiling.

    void FromJson(const Json::Value& j) override
    {
        perpWallDistClamp   = j.get("perpWallDistClamp",   perpWallDistClamp).asFloat();
        fogLutDistScale     = j.get("fogLutDistScale",     fogLutDistScale).asFloat();
        fogRebuildThreshold = j.get("fogRebuildThreshold", fogRebuildThreshold).asFloat();
        doorRayDirThreshold = j.get("doorRayDirThreshold", doorRayDirThreshold).asFloat();
        doorZBufferBias     = j.get("doorZBufferBias",     doorZBufferBias).asFloat();
        projectionNearClamp = j.get("projectionNearClamp", projectionNearClamp).asFloat();

        raycastColumnStep   = j.get("raycastColumnStep",   raycastColumnStep).asFloat();
        maxTrianglesPerMesh = j.get("maxTrianglesPerMesh", maxTrianglesPerMesh).asFloat();
        tilemapFbScale      = j.get("tilemapFbScale",      tilemapFbScale).asFloat();
        visibilityCullThreshold = j.get("visibilityCullThreshold", visibilityCullThreshold).asFloat();
        wallOverlapPadding  = j.get("wallOverlapPadding",  wallOverlapPadding).asFloat();
    }
};

/**
 * @class RendererSettings
 * @brief Static holder for shared rendering configuration values.
 */
class RendererSettings
{
private:
    RendererSettings() = delete;
    RendererSettings(const RendererSettings&) = delete;
    RendererSettings& operator=(const RendererSettings&) = delete;

public:
    /**
     * @brief Load rendering configuration from a JSON file.
     * @param configPath Path to rendering.json. Defaults to the engine config location.
     */
    static void Initialize(const std::string& configPath = "Assets/Config/Engine/rendering.json");

    static float perpWallDistClamp;   ///< @see RenderingConfig::perpWallDistClamp
    static float fogLutDistScale;     ///< @see RenderingConfig::fogLutDistScale
    static float fogRebuildThreshold; ///< @see RenderingConfig::fogRebuildThreshold
    static float doorRayDirThreshold; ///< @see RenderingConfig::doorRayDirThreshold
    static float doorZBufferBias;     ///< @see RenderingConfig::doorZBufferBias
    static float projectionNearClamp; ///< @see RenderingConfig::projectionNearClamp

    static int   raycastColumnStep;       ///< @see RenderingConfig::raycastColumnStep
    static int   maxTrianglesPerMesh;     ///< @see RenderingConfig::maxTrianglesPerMesh
    static float tilemapFbScale;          ///< @see RenderingConfig::tilemapFbScale
    static float visibilityCullThreshold; ///< @see RenderingConfig::visibilityCullThreshold
    static float wallOverlapPadding;      ///< @see RenderingConfig::wallOverlapPadding
};

} // namespace Engine
