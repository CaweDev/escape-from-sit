#include "Engine.hpp"
#include "Engine/Graphics/Camera.hpp"

namespace UI
{
    class Canvas;
    class Panel;
    class Text;
}

namespace Game
{
    class DebugMenu
    {
    public:
        DebugMenu() = default;
        ~DebugMenu() = default;

        void Initialize();
        void Update();
        void Shutdown();

        void Show();
        void Hide();
        void Toggle();

        bool IsVisible() const { return m_isVisible; }

    private:
        void BuildUI();

        bool m_isVisible = false;
        
        UI::Canvas* m_canvas = nullptr;
        UI::Panel*  m_panel = nullptr;
        UI::Panel*  m_cheatPanel = nullptr;  // wraps FPSScene-only cheat buttons
        UI::Text*   m_titleText = nullptr;
        UI::Text*   m_fpsText = nullptr;
        UI::Button* m_renderLimitBtn = nullptr;
        UI::Button* m_showCollidersBtn = nullptr;
        UI::Button* m_infiniteSpeedBtn = nullptr;
        UI::Button* m_catchImmuneBtn = nullptr;
        UI::Text*   m_instructionText = nullptr;
        
        float m_fpsUpdateTimer = 0.0f;
        
        bool m_showRenderLimit = false;
        bool m_isCursorUnlocked = false;
        Engine::Color m_originalCameraColor;
    };
}
