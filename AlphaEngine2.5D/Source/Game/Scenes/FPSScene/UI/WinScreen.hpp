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
        UI::Button* m_button = nullptr;
        bool m_visible = false;

    public:
        WinScreen() = default;
        ~WinScreen() override = default;

        void Awake() override;
        void OnDestroy() override;

        void Show();
        void Hide();
        bool IsVisible() const { return m_visible; }

    private:
        void BuildUI();
        void OnButtonClicked();
    };
}