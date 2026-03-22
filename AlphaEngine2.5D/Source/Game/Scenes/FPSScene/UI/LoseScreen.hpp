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
    class LoseScreen : public Engine::Component
    {
    private:
        UI::Canvas* m_canvas = nullptr;
        UI::Panel* m_overlay = nullptr;
        UI::Panel* m_panel = nullptr;
        UI::Text* m_title = nullptr;
        UI::Button* m_button = nullptr;          // Main Menu
        UI::Button* m_restartButton = nullptr;   // Restart
        bool m_visible = false;

        // Fade-in state
        bool m_isFadingIn = false;
        float m_fadeTimer = 0.0f;
        float m_fadeDuration = 0.22f;

    public:
        LoseScreen() = default;
        ~LoseScreen() override = default;

        void Awake() override;
        void Update() override;
        void OnDestroy() override;

        void Show();
        void Hide();
        bool IsVisible() const { return m_visible; }

    private:
        void BuildUI();
        void ApplyFade(float t);
        void OnButtonClicked();
        void OnRestartButtonClicked();
    };
}