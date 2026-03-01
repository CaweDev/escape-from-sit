#pragma once
/**
 * @file SplashScene.hpp
 * @brief Horror-themed DigiPen splash screen displayed on launch
 */

#include "Engine.hpp"

namespace Game {

using namespace Engine;
using namespace UI;

/**
 * @class SplashScene
 * @brief Splash screen that shows the DigiPen logo with horror effects
 *
 * Timeline: FadeIn -> HoldAndExpand -> Flicker (1 dip, swap to logo2) -> FinalFlicker (1 dip, swap to white) -> FinalHold -> FadeOut
 * Transitions automatically to MainMenu. Skippable via key/click.
 */
class SplashScene : public Scene {
public:
    SplashScene();

    void OnLoad() override;
    void OnUnload() override;
    void OnUpdate() override;

private:
    // Splash phases
    enum class Phase {
        FadeIn,        // UIManager fading out overlay to reveal logo
        HoldAndExpand, // Steady hold while logo slowly grows
        Flicker,       // Single quick flicker dip, swaps to logo2
        FinalFlicker,  // One last flicker dip, swaps back to white
        FinalHold,     // Brief hold on white logo before fade out
        FadeOut        // UIManager fading in overlay, then transition
    };

    // UI elements
    Canvas* m_canvas = nullptr;
    Panel*  m_background = nullptr;
    Image*  m_logo = nullptr;
    Image*  m_logo2 = nullptr;

    // State
    Phase m_phase = Phase::FadeIn;
    float m_phaseTimer = 0.0f;
    bool  m_transitioning = false;
    bool  m_flickerSwapped = false;

    // Phase durations
    static float FADE_IN_DURATION;
    static float HOLD_AND_EXPAND_DURATION;
    static float FINAL_HOLD_DURATION;

    // Canvas settings
    static float CANVAS_REF_WIDTH;
    static float CANVAS_REF_HEIGHT;
    static int   CANVAS_SORT_ORDER;

    // Logo settings
    static float LOGO_WIDTH;
    static float LOGO_HEIGHT;
    static int   LOGO_RENDER_ORDER;
    static float LOGO_EXPAND_SCALE;
    static const char* LOGO_WHITE_PATH;
    static const char* LOGO2_PATH;

    // Logo colors
    static Color LOGO_WHITE_COLOR;
    static Color LOGO2_COLOR;

    // Flicker settings
    static float FLICKER_DIP_DURATION;
    static float FLICKER_DEPTH;
    static float FLICKER_DURATION;
    static float FINAL_FLICKER_DURATION;

    void AdvancePhase();
    void SkipToEnd();
    bool IsSkipPressed() const;
};

} // namespace Game
