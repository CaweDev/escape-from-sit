/**
 * @file SpriteSheet.cpp
 * @brief Implementation of SpriteSheet class
 */
#include "SpriteSheet.hpp"
#include "../Core/AssetManager.hpp"

namespace Engine {

bool SpriteSheet::Load(const std::string& filepath, int rows, int cols) {
    if (rows <= 0 || cols <= 0) {
        printf("[Error] SpriteSheet: Invalid grid dimensions (%d x %d)\n", rows, cols);
        return false;
    }

    this->rows = rows;
    this->cols = cols;

    texture = AssetManager::GetTexture(filepath);
    if (!texture || !texture->IsLoaded()) {
        printf("[Error] SpriteSheet: Failed to load texture from %s\n", filepath.c_str());
        texture = nullptr;
        return false;
    }

    return true;
}

void SpriteSheet::AddAnimation(const std::string& name, int startFrame, int frameCount,
                                float fps, bool loop) {
    animations[name] = { name, startFrame, frameCount, fps, loop };
}

const AnimationClip* SpriteSheet::GetAnimation(const std::string& name) const {
    auto it = animations.find(name);
    if (it != animations.end()) {
        return &it->second;
    }
    return nullptr;
}

bool SpriteSheet::IsReady() const {
    return texture != nullptr && texture->IsLoaded() && rows > 0 && cols > 0;
}

} // namespace Engine
