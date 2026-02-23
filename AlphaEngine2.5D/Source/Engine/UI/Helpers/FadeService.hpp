#pragma once
/**
 * @file FadeService.hpp
 * @brief Static service for screen fade transitions
 *
 * Extracted from UIManager to keep concerns narrow.
 * Owns a dedicated high-sortOrder canvas with a full-screen Image overlay.
 */

#include "AEEngine.h"
#include <functional>

namespace UI
{
    class Canvas;
    class Image;

    /**
     * @class FadeService
     * @brief Static service for full-screen fade-to-black transitions
     *
     * Uses a dedicated Canvas (sortOrder 9999) with a full-screen black Image.
     * Supports smooth ease-in-out interpolation with completion callbacks.
     */
    class FadeService
    {
    public:
        /**
         * @brief Initialize the fade overlay (call after UIManager is ready)
         */
        static void Initialize();

        /**
         * @brief Shut down and release resources
         */
        static void Shutdown();

        /**
         * @brief Advance the fade animation (call once per frame)
         * @param dt Delta time in seconds
         */
        static void Update(f32 dt);

        /**
         * @brief Fade screen to black (transparent -> opaque)
         * @param duration Fade duration in seconds
         * @param onComplete Optional callback when fade completes
         */
        static void FadeIn(f32 duration, std::function<void()> onComplete = nullptr);

        /**
         * @brief Fade screen from black (opaque -> transparent)
         * @param duration Fade duration in seconds
         * @param onComplete Optional callback when fade completes
         */
        static void FadeOut(f32 duration, std::function<void()> onComplete = nullptr);

        /**
         * @brief Check if a fade is currently in progress
         */
        static bool IsFading() { return s_fading; }

    private:
        static Canvas* s_fadeCanvas;
        static Image*  s_fadeImage;

        static bool  s_fading;
        static f32   s_fadeAlpha;
        static f32   s_fadeFrom;
        static f32   s_fadeTo;
        static f32   s_fadeDuration;
        static f32   s_fadeElapsed;
        static std::function<void()> s_fadeCallback;
    };

} // namespace UI
