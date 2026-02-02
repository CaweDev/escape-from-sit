/**
 * @file DoorRenderer.hpp
 * @brief Renderer component for doors using raycasting projection
 */
#pragma once

#include "../Graphics/IRenderer.hpp"
#include "../Graphics/Sprite.hpp"
#include "../Graphics/Color.hpp"
#include "../Core/Component.hpp"
#include "AEEngine.h"
#include <memory>

namespace Engine {

// Forward declarations
class Door;

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
    void RenderMesh(AEGfxVertexList* mesh);
};

} // namespace Engine
