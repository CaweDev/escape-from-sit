/**
 * @file Sprite.hpp
 * @brief Sprite class wrapping AEGfxTexture and mesh
 */
#pragma once

#include "AEEngine.h"
#include "Texture2D.hpp"
#include <string>

namespace Engine {

/**
 * @class Sprite
 * @brief Represents a 2D sprite (texture + mesh)
 */
class Sprite {
public:
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
    
    /** Unload resources */
    void Unload();
    
    /** Check if loaded */
    bool IsLoaded() const;
    
    /** Get Alpha Engine texture */
    AEGfxTexture* GetTexture() const;
    
    /** Get Texture2D wrapper */
    Texture2D* GetTexture2D() const { return texture; }
    
    /** Get Alpha Engine mesh */
    AEGfxVertexList* GetMesh() const;

private:
    Texture2D* texture = nullptr;
    AEGfxVertexList* mesh = nullptr;

    
    void CreateQuadMesh();
    
public:
    /** Check if mesh is ready (texture optional) */
    bool IsReady() const;
};

} // namespace Engine
