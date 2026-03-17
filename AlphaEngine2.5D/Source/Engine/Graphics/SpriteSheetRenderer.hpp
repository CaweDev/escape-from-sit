/**
 * @file SpriteSheetRenderer.hpp
 * @brief Component for rendering animated spritesheets
 */
#pragma once

#include "../Core/Component.hpp"
#include "../Core/Transform.hpp"
#include "IRenderer.hpp"
#include "SpriteSheet.hpp"
#include "Color.hpp"
#include "../Raycasting/ProjectionHelper.hpp"
#include "Camera.hpp"
#include "AEEngine.h"
#include <Engine/Core/Application.hpp>
#include <string>

namespace Engine {

/**
 * @class SpriteSheetRenderer
 * @brief Component that renders animated sprites from a SpriteSheet
 *
 * References a shared SpriteSheet asset and manages per-instance playback
 * of named animation clips. Supports both 2D and 2.5D billboard rendering.
 */
class SpriteSheetRenderer : public IRenderer {
public:
    /** Color tint multiplier */
    Color color = Color::white();

    /** Sorting order for z-depth (higher = rendered later/on top) */
    int sortingOrder = 0;

    /** Flip sprite horizontally */
    bool flipX = false;

    /** Flip sprite vertically */
    bool flipY = false;

    /** If true, renders as a 2.5D billboard in raycast space */
    bool billboard = false;

    SpriteSheetRenderer();
    ~SpriteSheetRenderer();

    /** Set the color tint */
    void SetColor(const Color& newColor) { color = newColor; }

    /** Get the color tint */
    const Color& GetColor() const { return color; }

    /**
     * @brief Assign a spritesheet to this renderer
     * Rebuilds the internal mesh to match the sheet's grid dimensions.
     * @param sheet Pointer to a SpriteSheet (typically from AssetManager)
     */
    void SetSpriteSheet(SpriteSheet* sheet);

    /** @brief Get the currently assigned Spritesheet */
    const SpriteSheet* GetSpriteSheet() const { return spriteSheet; }

    /**
     * @brief Play a named animation from the start
     * @param animationName Name of the animation clip defined on the SpriteSheet
     */
    void Play(const std::string& animationName);

    /** @brief Stop playback and reset to frame 0 of the current clip */
    void Stop();

    /** @brief Pause playback at the current frame */
    void Pause();

    /** @brief Resume playback from paused state */
    void Resume();

    /** @brief Check if an animation is currently playing */
    bool IsPlaying() const { return playing; }

    /** @brief Get the name of the currently active animation clip */
    const std::string& GetCurrentAnimation() const;

    /**
     * @brief Manually set the frame index within the current clip
     * @param frame Frame index relative to the clip's start (0-based)
     */
    void SetFrame(int frame);

    /** @brief Get the current frame index within the clip */
    int GetFrame() const { return currentFrame; }

    /** @brief Set animation speed multiplier */
    void SetSpeed(float speed) { playbackSpeed = speed; }

    /** @brief Get animation speed multiplier */
    float GetSpeed() const { return playbackSpeed; }

    /** Render the sprite (called by renderer system) */
    void Render() override;

    /** Get sorting key for depth sorting */
    f32 GetSortingKey() const override;

private:
    SpriteSheet* spriteSheet = nullptr;
    AEGfxVertexList* mesh = nullptr;

    const AnimationClip* currentClip = nullptr;
    int currentFrame = 0;
    float elapsedTime = 0.0f;
    float playbackSpeed = 1.0f;
    bool playing = false;

    /** Build a 1x1 quad mesh with UVs sized to one cell of the spritesheet */
    void CreateSpritesheetMesh();

    /** Free the internal mesh */
    void FreeMesh();

    /** Add a visible strip to the dynamic billboard mesh */
    void AddStripToMesh(int x1, int x2, f32 spriteStart, f32 spriteWidth,
                        f32 yTop, f32 yBottom, f32 uOffsetX, f32 uOffsetY,
                        f32 cellU, f32 cellV);

    static const std::string s_emptyString;
};

} // namespace Engine
