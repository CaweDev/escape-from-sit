/**
 * @file SpriteSheet.hpp
 * @brief Spritesheet data asset with named animation clips
 */
#pragma once

#include "Texture2D.hpp"
#include <string>
#include <unordered_map>

namespace Engine {

/**
 * @struct AnimationClip
 * @brief Defines a named animation as a range of frames within a spritesheet
 *
 * Frames are indexed in row-major order (left-to-right, top-to-bottom),
 * starting from 0 at the top-left cell of the spritesheet.
 */
struct AnimationClip {
    std::string name;
    int startFrame = 0;    ///< First frame index (row-major, 0-based). Read from meta file later
    int frameCount = 1;    ///< Number of frames in this clip. Read from meta file later
    float frameRate = 10.0f; ///< Playback speed (frames per second). Read from meta file later
    bool loop = true;      ///< Whether this clip loops. Read from meta file later
};

/**
 * @class SpriteSheet
 * @brief Shared data asset representing a spritesheet with named animations
 *
 * Holds a texture reference, grid layout (rows x cols), and a map of named
 * AnimationClips. Multiple SpriteSheetRenderers can share the same SpriteSheet
 * while each playing different animations independently.
 *
 * Cached by AssetManager via GetSpriteSheet().
 */
class SpriteSheet {
public:
    SpriteSheet() = default;
    ~SpriteSheet() = default;

    // Non-copyable (shared asset)
    SpriteSheet(const SpriteSheet&) = delete;
    SpriteSheet& operator=(const SpriteSheet&) = delete;

    /**
     * @brief Load texture and set grid dimensions
     * @param filepath Path to spritesheet image (PNG)
     * @param rows Number of rows in the spritesheet grid
     * @param cols Number of columns in the spritesheet grid
     * @return True if texture loaded successfully
     */
    bool Load(const std::string& filepath, int rows, int cols);

    /**
     * @brief Define or overwrite a named animation clip
     *
     * If an animation with the same name already exists, it is silently
     * overwritten. Frame indices are row-major (0 = top-left cell).
     *
     * @param name    Unique name for this animation (e.g. "idle", "walk")
     * @param startFrame  Index of the first frame in the clip
     * @param frameCount  Number of frames in the clip
     * @param fps     Playback speed in frames per second
     * @param loop    Whether the animation loops (default: true)
     */
    void AddAnimation(const std::string& name, int startFrame, int frameCount,
                      float fps, bool loop = true);

    /** @brief Get an animation clip by name, or nullptr if not found */
    const AnimationClip* GetAnimation(const std::string& name) const;

    /** @brief Get the underlying texture */
    Texture2D* GetTexture() const { return texture; }

    /** @brief Get grid row count */
    int GetRows() const { return rows; }

    /** @brief Get grid column count */
    int GetCols() const { return cols; }

    /** @brief Get total cells in the grid (rows * cols) */
    int GetTotalCells() const { return rows * cols; }

    /** @brief Check if texture is loaded and grid is valid */
    bool IsReady() const;

private:
    Texture2D* texture = nullptr; ///< Non-owning; managed by AssetManager
    int rows = 1;
    int cols = 1;
    std::unordered_map<std::string, AnimationClip> animations;
};

} // namespace Engine
