/**
 * @file Sprite.hpp
 * @brief Sprite class wrapping AEGfxTexture and mesh
 */
#pragma once

#include "AEEngine.h"
#include <string>

namespace Engine {

/**
 * @class Sprite
 * @brief Represents a 2D sprite (texture + mesh)
 */
class Sprite {
public:
    /** Width of the sprite in pixels */
    int width = 0;
    
    /** Height of the sprite in pixels */
    int height = 0;
    
    /** Pixels per unit (for scaling) */
    f32 pixelsPerUnit = 100.0f;

    Sprite();
    ~Sprite();

    // Prevent copying (owns resources)
    Sprite(const Sprite&) = delete;
    Sprite& operator=(const Sprite&) = delete;
    
    // Allow moving
    Sprite(Sprite&& other) noexcept;
    Sprite& operator=(Sprite&& other) noexcept;

    /** Load sprite from file */
    bool Load(const std::string& filepath);
    
    /** Create sprite with dimensions */
    bool Load(const std::string& filepath, int w, int h);
    
    /** Unload resources */
    void Unload();
    
    /** Check if loaded */
    bool IsLoaded() const;
    
    /** Get Alpha Engine texture */
    AEGfxTexture* GetTexture() const;
    
    /** Get Alpha Engine mesh */
    AEGfxVertexList* GetMesh() const;

private:
    AEGfxTexture* texture = nullptr;
    AEGfxVertexList* mesh = nullptr;
    
    void CreateQuadMesh();
    
public:
    /** Create a colored quad without texture (sets pixelsPerUnit to 1 for direct pixel sizing) */
    void CreateMeshOnly(int w = 100, int h = 100);
    
    /** Check if mesh is ready (texture optional) */
    bool IsReady() const;
};

} // namespace Engine
