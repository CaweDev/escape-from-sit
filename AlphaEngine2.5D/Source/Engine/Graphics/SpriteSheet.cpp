/**
 * @file SpriteSheet.cpp
 * @brief Implementation of SpriteSheet class
 */
#include "SpriteSheet.hpp"
#include "../Core/AssetManager.hpp"

namespace Engine {

/*!
 * \brief [AUTO] Load
 * \param filepath [in]
 * \param numRows [in]
 * \param numCols [in]
 * \return 
 */
/*!
 * \brief [AUTO] Load
 * \param filepath [in]
 * \param numRows [in]
 * \param numCols [in]
 * \return 
 */
bool SpriteSheet::Load(const std::string& filepath, int numRows, int numCols) {
    if (numRows <= 0 || numCols <= 0) {
        printf("[Error] SpriteSheet: Invalid grid dimensions (%d x %d)\n", numRows, numCols);
        return false;
    }

    this->rows = numRows;
    this->cols = numCols;

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

/*!
 * \brief [AUTO] GetAnimation
 * \param name [in]
 * \return 
 */
/*!
 * \brief [AUTO] GetAnimation
 * \param name [in]
 * \return 
 */
const AnimationClip* SpriteSheet::GetAnimation(const std::string& name) const {
    auto it = animations.find(name);
    if (it != animations.end()) {
        return &it->second;
    }
    return nullptr;
}

/*!
 * \brief [AUTO] IsReady
 * \return 
 */
/*!
 * \brief [AUTO] IsReady
 * \return 
 */
bool SpriteSheet::IsReady() const {
    return texture != nullptr && texture->IsLoaded() && rows > 0 && cols > 0;
}

} // namespace Engine
