/**
 * @file SplashScene.cpp
 * @brief Implementation of horror-themed DigiPen splash screen
 */
#include "SplashScene.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/UI/Core/UIManager.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/AssetManager.hpp"
#include "Engine/Graphics/Color.hpp"
#include "Engine/Input/Input.hpp"
#include <cmath>
#include <cstdlib>

namespace Game {

// Phase durations
float SplashScene::FADE_IN_DURATION        = 1.2f;
float SplashScene::HOLD_AND_EXPAND_DURATION = 2.f;
float SplashScene::FINAL_HOLD_DURATION     = 1.0f;

// Canvas settings
float SplashScene::CANVAS_REF_WIDTH  = 1600.f;
float SplashScene::CANVAS_REF_HEIGHT = 900.f;
int   SplashScene::CANVAS_SORT_ORDER = 100;

// Logo settings
int   SplashScene::LOGO_RENDER_ORDER = 1;
float SplashScene::LOGO_EXPAND_SCALE = 1.05f;

float SplashScene::LOGO_WIDTH        = 600.0f;
float SplashScene::LOGO_HEIGHT       = 200.0f;
const char* SplashScene::LOGO_WHITE_PATH = "Assets/UI/DigiPen_BLACK.png";
float SplashScene::LOGO2_WIDTH        = 600.0f;
float SplashScene::LOGO2_HEIGHT       = 600.0f;
const char* SplashScene::LOGO2_PATH      = "Assets/xav.jpg";

// Logo colors
Color SplashScene::LOGO_WHITE_COLOR = Color(1.0f, 1.0f, 1.0f, 1.0f);
Color SplashScene::LOGO2_COLOR      = Color(1.0f, 1.0f, 1.0f, 1.0f);

// Flicker settings
float SplashScene::FLICKER_DIP_DURATION   = 0.2f;
float SplashScene::FLICKER_DEPTH          = 1.f;
float SplashScene::FLICKER_DURATION       = 0.2f;
float SplashScene::FINAL_FLICKER_DURATION = 0.2f;

SplashScene::SplashScene() : Scene("SplashScene") {}

void SplashScene::OnLoad() {
    // Seed random for flicker variation
    srand(static_cast<unsigned>(time(nullptr)));

    // Reset state
    m_phase = Phase::FadeIn;
    m_phaseTimer = 0.0f;
    m_transitioning = false;

 

    // Canvas
    m_canvas = UIManager::GetInstance()->CreateCanvas();
    m_canvas->SetReferenceResolution(CANVAS_REF_WIDTH, CANVAS_REF_HEIGHT);
    m_canvas->SetScaleMode(CanvasScaleMode::ScaleWithScreenSize);
    m_canvas->SetSortOrder(CANVAS_SORT_ORDER);

    // Black Background
    m_background = m_canvas->CreateChild<Panel>();
    m_background->name = "SplashBg";
    m_background->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
    m_background->SetAnchoredPosition({ 0.0f, 0.0f });
    m_background->SetSizeDelta({ 0.0f, 0.0f });
    m_background->SetBackgroundColor(Color(0.0f, 0.0f, 0.0f, 1.0f));

    // DigiPen Logo (centered)
    m_logo = m_canvas->CreateChild<Image>();
    m_logo->name = "DigiPenLogo";

    // Center the logo
    m_logo->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
    m_logo->SetPivot({ 0.5f, 0.5f });
    m_logo->SetAnchoredPosition({ 0.0f, 0.0f });
    m_logo->SetSizeDelta({ LOGO_WIDTH, LOGO_HEIGHT });
    {
        // Load logo textures
        Texture2D* tex = AssetManager::GetTexture(LOGO_WHITE_PATH);
        m_logo->SetTexture(tex);
    }
    m_logo->SetColor(Color(1.0f, 1.0f, 1.0f, 0.0f));
    m_logo->SetRenderOrder(LOGO_RENDER_ORDER);
    m_logo->SetActive(true);

    // Second Logo
    m_logo2 = m_canvas->CreateChild<Image>();
    m_logo2->name = "DigiPenLogo2";
    m_logo2->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
    m_logo2->SetPivot({ 0.5f, 0.5f });
    m_logo2->SetAnchoredPosition({ 0.0f, 0.0f });
    m_logo2->SetSizeDelta({ 600, 600 });
    {
        Texture2D* tex = AssetManager::GetTexture(LOGO2_PATH);
        m_logo2->SetTexture(tex);
    }
    m_logo2->SetColor(Color(LOGO2_COLOR.r, LOGO2_COLOR.g, LOGO2_COLOR.b, 0.0f));
    m_logo2->SetRenderOrder(LOGO_RENDER_ORDER);
    m_logo2->SetActive(false);

    // Start with screen fully black (fade overlay at alpha = 1)
    // Then fade it out to reveal our scene
    FadeService::FadeOut(FADE_IN_DURATION);
}

void SplashScene::OnUpdate() {
    float dt = Time::unscaledDeltaTime;
    m_phaseTimer += dt;

    // Skip check — any key or mouse click
    if (!m_transitioning && IsSkipPressed()) {
        SkipToEnd();
        return;
    }

    switch (m_phase) {
    // FADE IN: UIManager is fading out its overlay. Logo ramps up alpha.
    case Phase::FadeIn: {
        float t = m_phaseTimer / FADE_IN_DURATION;
        if (t > 1.0f) t = 1.0f;

        const_cast<Color&>(m_logo->GetColor()).a = t;
        const_cast<Color&>(m_logo2->GetColor()).a = t;

        if (m_phaseTimer >= FADE_IN_DURATION) {
            AdvancePhase();
        }
        break;
    }

    // HOLD AND EXPAND: Stable white logo that slowly grows
    case Phase::HoldAndExpand: {
        float t = m_phaseTimer / HOLD_AND_EXPAND_DURATION;
        if (t > 1.0f) t = 1.0f;

        // Smoothstep for a gentle ease-in-out
        float smooth = t * t * (3.0f - 2.0f * t);
        float scale = 1.0f + (LOGO_EXPAND_SCALE - 1.0f) * smooth;

        m_logo->SetSizeDelta({ LOGO_WIDTH * scale, LOGO_HEIGHT * scale });
        m_logo2->SetSizeDelta({ LOGO2_WIDTH * scale, LOGO2_HEIGHT * scale });

        if (m_phaseTimer >= HOLD_AND_EXPAND_DURATION) {
            AdvancePhase();
        }
        break;
    }

    // FLICKER: Single quick flicker dip, swaps to logo2 at midpoint
    case Phase::Flicker: {
        float t = m_phaseTimer;

        auto flickerPulse = [](float local) {
            float clamped = local < 0.0f ? 0.0f : (local > 1.0f ? 1.0f : local);
            return 1.0f - FLICKER_DEPTH * sinf(clamped * 3.14159265f);
        };

        float flicker = 1.0f;
        if (t < FLICKER_DIP_DURATION) {
            float local = t / FLICKER_DIP_DURATION;
            flicker = flickerPulse(local);

            // Swap at midpoint when logo is fully transparent
            if (!m_flickerSwapped && local >= 0.5f) {
                m_flickerSwapped = true;
                m_logo->SetActive(false);
                m_logo2->SetActive(true);
            }
        }

        const_cast<Color&>(m_logo->GetColor()).a = flicker;
        const_cast<Color&>(m_logo2->GetColor()).a = flicker;

        if (m_phaseTimer >= FLICKER_DURATION) {
            AdvancePhase();
        }
        break;
    }

    // FINAL FLICKER: One last dip, swaps back to white logo at midpoint
    case Phase::FinalFlicker: {
        float t = m_phaseTimer;

        auto flickerPulse = [](float local) {
            float clamped = local < 0.0f ? 0.0f : (local > 1.0f ? 1.0f : local);
            return 1.0f - FLICKER_DEPTH * sinf(clamped * 3.14159265f);
        };

        float flicker = 1.0f;
        if (t < FLICKER_DIP_DURATION) {
            float local = t / FLICKER_DIP_DURATION;
            flicker = flickerPulse(local);

            // Swap at midpoint when logo is fully transparent
            if (!m_flickerSwapped && local >= 0.5f) {
                m_flickerSwapped = true;
                m_logo->SetActive(true);
                m_logo2->SetActive(false);
            }
        }

        const_cast<Color&>(m_logo->GetColor()).a = flicker;
        const_cast<Color&>(m_logo2->GetColor()).a = flicker;

        if (m_phaseTimer >= FINAL_FLICKER_DURATION) {
            AdvancePhase();
        }
        break;
    }

    // FINAL HOLD: Brief hold on white logo before transitioning
    case Phase::FinalHold: {
        if (m_phaseTimer >= FINAL_HOLD_DURATION) {
            AdvancePhase();
        }
        break;
    }
    }
}

void SplashScene::AdvancePhase() {
    m_phaseTimer = 0.0f;

    switch (m_phase) {
    case Phase::FadeIn:
        m_phase = Phase::HoldAndExpand;
        break;

    case Phase::HoldAndExpand:
        m_phase = Phase::Flicker;
        m_flickerSwapped = false;
        break;

    case Phase::Flicker:
        m_phase = Phase::FinalFlicker;
        m_flickerSwapped = false;
        break;

    case Phase::FinalFlicker:
        m_phase = Phase::FinalHold;
        break;

    case Phase::FinalHold:
        m_phase = Phase::FadeOut;
        m_transitioning = true;
		SceneManager::LoadWithFade("MainMenu");
        break;

    case Phase::FadeOut:
        break;
    }
}

void SplashScene::SkipToEnd() {
    if (m_transitioning) return;
    m_transitioning = true;

    SceneManager::LoadWithFade("MainMenu");
}

bool SplashScene::IsSkipPressed() const {
    // Check any common keys
    if (Input::GetKeyDown(KeyCode::Return) ||
        Input::GetKeyDown(KeyCode::Space) ||
        Input::GetMouseButtonDown(0) ||
        Input::GetMouseButtonDown(1)) {
        return true;
    }
    return false;
}

void SplashScene::OnUnload() {
    if (m_canvas) {
        auto* uiMgr = UIManager::GetInstance();
        if (uiMgr) {
            uiMgr->DestroyCanvas(m_canvas);
        }
        m_canvas = nullptr;
    }
    m_background = nullptr;
    m_logo = nullptr;
    m_logo2 = nullptr;
}

} // namespace Game
