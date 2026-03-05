/**
 * @file PauseMenu.hpp
 * @brief In-game pause menu component with school-horror themed UI
 *
 * Toggled with Escape. Pauses the game, unlocks the cursor, and presents
 * Resume / Restart / Main Menu options over a dark overlay.
 */
#pragma once

#include "Engine.hpp"
#include <functional>
#include <vector>
#include <string>

// Forward declarations
namespace UI
{
    class Canvas;
    class Panel;
    class Text;
    class Button;
    class Image;
}

namespace Game
{
    /**
     * @class PauseMenu
     * @brief Escape-key pause overlay with Resume, Restart, and Main Menu
     *
     * Attach to the Player GameObject (same as PlayerHUD).
     * Manages its own Canvas at a high sort-order so it renders above the HUD.
     *
     * Hierarchy:
     *   Canvas (sort 500)
     *   └── Panel (overlay – full-screen dim)
     *       └── Panel (menu – centered box)
     *           ├── Text  ("PAUSED")
     *           ├── Image (divider)
     *           ├── Button (Resume)
     *           ├── Button (Restart)
     *           └── Button (Main Menu)
     */
    class PauseMenu : public Engine::Component
    {
    private:
        // ── UI elements (owned by Canvas → UIManager) ──
        UI::Canvas* m_canvas       = nullptr;
        UI::Panel*  m_overlay      = nullptr;
        UI::Panel*  m_menuPanel    = nullptr;
        UI::Text*   m_title        = nullptr;
        UI::Image*  m_divider      = nullptr;
        UI::Button* m_resumeBtn    = nullptr;
        UI::Button* m_restartBtn   = nullptr;
        UI::Button* m_mainMenuBtn  = nullptr;

        // ── State ──
        bool m_isPaused       = false;
        int  m_selectedIndex  = 0;
        int  m_hoveredIndex   = -1;
        int  m_pressedIndex   = -1;

        // ── Menu item tracking (for keyboard nav + visual updates) ──
        struct MenuItem
        {
            UI::Button*  button     = nullptr;
            std::string  label;
            float        baseFontSize = 0.f;
            float        currentFontSize = 0.f;
            float        targetFontSize  = 0.f;
        };
        std::vector<MenuItem> m_items;

    public:
        PauseMenu() = default;
        virtual ~PauseMenu();

        // ── Component lifecycle ──
        void Awake()     override;
        void Update()    override;
        void OnDestroy() override;

        // ── Public API ──
        void Show();
        void Hide();
        void Toggle();
        bool IsPaused() const { return m_isPaused; }

    private:
        // ── Internal helpers ──
        void BuildUI();
        void UpdateMenuVisuals();
        void UpdateMenuAnimations(float dt);
        void SelectMenuItem(int index);

        // ── Button callbacks ──
        void OnResumeClicked();
        void OnRestartClicked();
        void OnMainMenuClicked();
    };

} // namespace Game
