#pragma once

/**
 * @file LoadingScene.hpp
 * @brief Loading screen scene displayed during scene transitions
 */

#include "Engine.hpp"
#include <string>

namespace Game {

    using namespace Engine;
    using namespace UI;

    /**
     * @class LoadingScene
     * @brief Intermediate scene that displays loading UI between scene transitions
     *
     * Used by SceneManager::LoadWithScreen. Shows a "LOADING..." text with
     * animated dots and a progress bar, then deferred-loads the target scene.
     */
    class LoadingScene : public Scene {
    public:
        LoadingScene();

        void OnLoad() override;
        void OnUnload() override;
        void OnUpdate() override;

    private:
        // UI elements
        Canvas* m_canvas = nullptr;
        Panel*  m_background = nullptr;
        Text*   m_loadingText = nullptr;
        Image*  m_progressBarBg = nullptr;
        Image*  m_progressBarFill = nullptr;

        // Animation state
        float m_elapsed = 0.0f;
        float m_dotTimer = 0.0f;
        int   m_dotCount = 0;
        bool  m_loadTriggered = false;

        static constexpr float MIN_DISPLAY_TIME = 1.2f;
        static constexpr float PROGRESS_DURATION = 1.2f;
        static constexpr float DOT_INTERVAL = 0.3f;
    };

} // namespace Game
