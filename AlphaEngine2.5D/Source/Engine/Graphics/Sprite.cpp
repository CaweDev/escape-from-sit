/**
 * @file Sprite.cpp
 * @brief Implementation of Sprite class
 */
#include "Sprite.hpp"
#include "../Core/AssetManager.hpp"

namespace Engine {

Sprite::Sprite() : width(0), height(0), pixelsPerUnit(100.0f), texture(nullptr), mesh(nullptr) {}

Sprite::~Sprite() {
    Unload();
}

Sprite::Sprite(Sprite&& other) noexcept {
    texture = other.texture;
    mesh = other.mesh;
    width = other.width;
    height = other.height;
    pixelsPerUnit = other.pixelsPerUnit;
    other.texture = nullptr;
    other.mesh = nullptr;
}

Sprite& Sprite::operator=(Sprite&& other) noexcept {
    if (this != &other) {
        Unload();
        texture = other.texture;
        mesh = other.mesh;
        width = other.width;
        height = other.height;
        pixelsPerUnit = other.pixelsPerUnit;
        other.texture = nullptr;
        other.mesh = nullptr;
    }
    return *this;
}

bool Sprite::Load(const std::string& filepath) {
    Unload();
    
    // Create quad mesh (1x1 unit, centered) regardless of texture success
    CreateQuadMesh();
    
    // Use AssetManager to get cached texture (non-owning pointer)
    texture = AssetManager::GetTexture(filepath);
    if (!texture || !texture->IsLoaded()) {
        printf("[Error] Sprite: Failed to load texture from %s. Rendering as fallback quad.\n", filepath.c_str());
        // We still have the mesh, so we can render a color quad if needed
        return true; 
    }
    
    return true;
}

bool Sprite::Load(const std::string& filepath, int w, int h) {
    width = w;
    height = h;
    return Load(filepath);
}

void Sprite::Unload() {
    // Texture is now managed by AssetManager, don't delete it
    texture = nullptr;
    
    if (mesh) {
        AEGfxMeshFree(mesh);
        mesh = nullptr;
    }
}

bool Sprite::IsLoaded() const {
    return texture != nullptr && texture->IsLoaded() && mesh != nullptr;
}

AEGfxTexture* Sprite::GetTexture() const {
    return texture ? texture->GetNativeTexture() : nullptr;
}


AEGfxVertexList* Sprite::GetMesh() const {
    return mesh;
}

void Sprite::CreateQuadMesh() {
    if (mesh) {
        AEGfxMeshFree(mesh);
        mesh = nullptr;
    }
    
    // Create 1x1 unit quad centered at origin
    AEGfxMeshStart();
    
    // Two triangles forming a quad
    // Triangle 1: bottom-left, top-left, top-right
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,  // bottom-left
         -0.5f,  0.5f, 0xFFFFFFFF, 0.0f, 0.0f,  // top-left
          0.5f,  0.5f, 0xFFFFFFFF, 1.0f, 0.0f   // top-right
    );
    
    // Triangle 2: bottom-left, top-right, bottom-right
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,  // bottom-left
         0.5f,  0.5f, 0xFFFFFFFF, 1.0f, 0.0f,  // top-right
         0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f   // bottom-right
    );
    
    mesh = AEGfxMeshEnd();
}

void Sprite::CreateMeshOnly(int w, int h) {
    width = w;
    height = h;
    pixelsPerUnit = 1.0f;  // Direct pixel sizing for colored quads
    CreateQuadMesh();
}

bool Sprite::IsReady() const {
    return mesh != nullptr;
}

} // namespace Engine
