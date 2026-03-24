/**
 * @file DoorRenderer.hpp
 * @brief Renderer component for doors using raycasting projection
 */
#pragma once

#include "../Graphics/IRenderer.hpp"
#include "../Graphics/Sprite.hpp"
#include "../Graphics/Color.hpp"
#include "../Core/Component.hpp"
#include "../Core/ConfigManager.hpp"
#include "AEEngine.h"
#include <memory>
#include <string>

namespace Engine {

// Forward declarations
class Door;

/**
 * @brief IConfig implementation for DoorRenderer, deserialised from door.json.
 */
struct DoorRendererConfig : public IConfig
{
    Color colorNS = Color(0.6f, 0.4f, 0.2f, 1.0f);
    Color colorEW = Color(0.5f, 0.35f, 0.18f, 1.0f);
    float fov     = 75.0f;

    void FromJson(const Json::Value& j) override
    {
        if (j.isMember("rendererColorNS")) {
            const auto& c = j["rendererColorNS"];
            colorNS = Color(c.get("r", colorNS.r).asFloat(),
                            c.get("g", colorNS.g).asFloat(),
                            c.get("b", colorNS.b).asFloat(),
                            c.get("a", colorNS.a).asFloat());
        }
        if (j.isMember("rendererColorEW")) {
            const auto& c = j["rendererColorEW"];
            colorEW = Color(c.get("r", colorEW.r).asFloat(),
                            c.get("g", colorEW.g).asFloat(),
                            c.get("b", colorEW.b).asFloat(),
                            c.get("a", colorEW.a).asFloat());
        }
        fov     = j.get("rendererFov", fov).asFloat();
    }
};

/**
 * @class DoorRenderer
 * @brief Renders a door as a wall segment using raycasting projection
 * 
 * The door is rendered as a vertical wall segment that slides vertically
 * based on the door's openAmount. When fully open, nothing renders.
 */
class DoorRenderer : public IRenderer {
public:
    REQUIRE_COMPONENT(Door)
    
    /** Sprite for the door texture (optional) */
    Sprite* sprite;
    
    /** Fallback color for North/South facing doors */
    Color colorNS = Color(0.6f, 0.4f, 0.2f, 1.0f);
    
    /** Fallback color for East/West facing doors */
    Color colorEW = Color(0.5f, 0.35f, 0.18f, 1.0f);
    
    /** Field of view (should match TileMapRenderer) */
    f32 fov = 75.0f;
    
    DoorRenderer();
    virtual ~DoorRenderer();

    /**
     * @brief Load DoorRenderer defaults from door.json.
     * Call once at startup; newly created DoorRenderers will pick up these defaults.
     */
    static void Initialize(const std::string& configPath = "Assets/Config/Engine/door.json");
    
    /** Render the door */
    void Render() override;
    
    /** Get sorting key (distance from camera, for depth sorting) */
    f32 GetSortingKey() const override;
    
private:
    /** Cached reference to Door component */
    Door* door = nullptr;
    
    /** Mesh for rendering door quad */
    AEGfxVertexList* doorMesh = nullptr;
    
    /** Cache references on first render */
    void CacheReferences();
    
    /** Create the quad mesh */
    void CreateMesh();
    
    /** Calculate perpendicular distance from camera to door */
    f32 CalculatePerpDistance() const;

    /** Helper to render a batched mesh */
    void RenderMesh(AEGfxVertexList* mesh, f32 fogFactor);
};

} // namespace Engine
