/*!
 * \file AlphaEngine2.5D\Source\Game\Scenes\MainMenuScene\UI\ControlsMenu.hpp
 * \brief Controls Menu UI implementation for displaying game controls
 */

#pragma once

#include "Engine.hpp"
#include <functional>

namespace UI
{
    class Canvas;
    class Panel;
    class Image;
    class Button;
}

namespace Game
{
    /**
     * @class ControlsMenu
     * @brief A simple overlay menu to display the controls image.
     */
    class ControlsMenu : public Engine::Component
    {
    private:
        UI::Canvas* m_canvas       = nullptr;
        UI::Panel*  m_overlay      = nullptr;
        UI::Image*  m_panelImage   = nullptr;
        UI::Image*  m_controlsImage = nullptr;
        UI::Button* m_backBtn      = nullptr;

        bool m_isVisible = false;
        
        // For button hover/click state
        bool m_isHovered = false;
        bool m_isPressed = false;

    public:
        ControlsMenu() = default;
        virtual ~ControlsMenu();

        void Awake()     override;
        void Update()    override;
        void OnDestroy() override;

        void Show();
        void Hide();
        void Toggle();
        bool IsVisible() const { return m_isVisible; }

    private:
        void BuildUI();
        void OnBackClicked();
        void UpdateButtonVisuals();
    };

} // namespace Game
