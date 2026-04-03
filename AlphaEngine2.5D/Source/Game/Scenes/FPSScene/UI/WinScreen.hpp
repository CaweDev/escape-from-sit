/*!
 * \file AlphaEngine2.5D\Source\Game\Scenes\FPSScene\UI\WinScreen.hpp
 * \brief ui component for the win screen that appears after beating the game, showing stats and a button to return to the main menu
 */

#pragma once

#include "Engine.hpp"

namespace UI
{
    class Canvas;
    class Panel;
    class Text;
    class Button;
}

namespace Game
{
    class WinScreen : public Engine::Component
    {
    private:
        UI::Canvas* m_canvas = nullptr;
        UI::Panel* m_overlay = nullptr;
        UI::Panel* m_panel = nullptr;
        UI::Text* m_title = nullptr;
        UI::Text* m_timeSpentText = nullptr;
        UI::Text* m_highScoreText = nullptr;
        UI::Button* m_button = nullptr;
        bool m_visible = false;

        // Fade state
        bool m_isFadingIn = false;
        bool m_isFadingOut = false;
        float m_fadeTimer = 0.0f;
        float m_fadeDuration = 0.5f;

        bool m_loadMainMenuAfterFade = false;

    public:
        WinScreen() = default;
        ~WinScreen() override = default;

        void Awake() override;
        void Update() override;
        void OnDestroy() override;

        void Show();
        void Hide();
        bool IsVisible() const { return m_visible; }

    private:
        void BuildUI();
        void ApplyFade(float t);
        void CompleteHide();
        void OnButtonClicked();
    };
}