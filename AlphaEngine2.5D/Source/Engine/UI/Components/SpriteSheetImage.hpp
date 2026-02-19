#pragma once
/**
 * @file AnimatedImage.hpp
 * @brief UI Image component with spritesheet animation support
 */

#include "Engine/UI/Components/Image.hpp"
#include "Engine/Graphics/SpriteSheet.hpp"
#include <string>

namespace UI
{
    /**
	 * @class SpriteSheetImage
     * @brief UI component for rendering animated spritesheets
     *
     * Extends Image to support spritesheet-based animation. References a shared
     * SpriteSheet for animation data and manages per-instance playback state.
     */
    class SpriteSheetImage : public UIElement
    {
    private:
        Engine::SpriteSheet* m_spriteSheet = nullptr;
        AEGfxVertexList* m_mesh = nullptr;  ///< Owned mesh with 1/c x 1/r UVs
        Engine::Color m_color;

        const Engine::AnimationClip* m_currentClip = nullptr;
        int m_currentFrame = 0;
        float m_elapsedTime = 0.0f;
        bool m_playing = false;

        static const std::string s_emptyString;

        /** Build a 1x1 quad mesh with UVs sized to one cell */
        void CreateSpritesheetMesh();

        /** Free the internal mesh */
        void FreeMesh();

    public:
        SpriteSheetImage();
        virtual ~SpriteSheetImage();

        /**
         * @brief Assign a spritesheet (rebuilds internal mesh)
         * @param sheet Pointer to a SpriteSheet (typically from AssetManager)
         */
        void SetSpriteSheet(Engine::SpriteSheet* sheet);

        /**
         * @brief Play a named animation from the start
         * @param animationName Name of the clip defined on the SpriteSheet
         */
        void Play(const std::string& animationName);

        /** @brief Stop playback and reset to frame 0 */
        void Stop();

        /** @brief Pause at current frame */
        void Pause();

        /** @brief Resume from paused state */
        void Resume();

        /** @brief Check if playing */
        bool IsPlaying() const { return m_playing; }

        /** @brief Get name of active animation clip */
        const std::string& GetCurrentAnimation() const;

        /**
         * @brief Manually set frame index within the current clip
         * @param frame Frame index relative to clip start (0-based)
         */
        void SetFrame(int frame);

        /** @brief Get current frame index within clip */
        int GetFrame() const { return m_currentFrame; }

        /**
         * @brief Set the color (tint if texture, solid if no texture)
         */
        void SetColor(const Engine::Color& color) { m_color = color; }
        const Engine::Color& GetColor() const { return m_color; }

        /** @brief Update animation timing */
        void Update(f32 dt) override;

        /** @brief Render the current animation frame */
        void Render() override;
    };

} // namespace UI
