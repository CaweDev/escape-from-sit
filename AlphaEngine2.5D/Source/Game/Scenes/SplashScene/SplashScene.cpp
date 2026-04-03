/*!
 * \file SplashScene.cpp
 * \brief Implementation of horror-themed DigiPen splash screen
 */
#include "SplashScene.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/UI/Core/UIManager.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/AssetManager.hpp"
#include "Engine/Graphics/Color.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <cmath>
#include <cstdlib>

namespace
{
    /*!
     * \brief Try to read a 2D vector from JSON node
     * \param node [in] JSON node to read from
     * \param key [in] Key name to lookup in node
     * \param out [out] Output vector written on success
     * \return true if a valid object was read, false otherwise
     */
    bool TryReadVec2(const Json::Value& node, const char* key, AEVec2& out)
    {
        if (!node.isMember(key) || !node[key].isObject()) return false;
        const auto& v = node[key];
        out.x = v.get("x", out.x).asFloat();
        out.y = v.get("y", out.y).asFloat();
        return true;
    }

    /*!
     * \brief Apply saved layout values from JSON to a UI element
     * \param e [in/out] UI element to configure
     * \param layout [in] JSON layout object containing overrides
     */
    void ApplyLayoutNode(UI::UIElement* e, const Json::Value& layout)
    {
        if (!e || e->name.empty() || !layout.isObject()) return;
        const auto& n = layout[e->name];
        if (!n.isObject()) return;

        AEVec2 v{};
        v = e->GetAnchoredPosition(); if (TryReadVec2(n, "anchoredPosition", v)) e->SetAnchoredPosition(v);
        v = e->GetSizeDelta();        if (TryReadVec2(n, "sizeDelta", v))        e->SetSizeDelta(v);
        v = e->GetPivot();            if (TryReadVec2(n, "pivot", v))            e->SetPivot(v);
        AEVec2 amin = e->GetAnchorMin(), amax = e->GetAnchorMax();
        bool hasMin = TryReadVec2(n, "anchorMin", amin);
        bool hasMax = TryReadVec2(n, "anchorMax", amax);
        if (hasMin || hasMax) e->SetAnchors(amin, amax);
        if (n.isMember("renderOrder")) e->SetRenderOrder(n["renderOrder"].asInt());
    }

    /*!
     * \struct SplashSceneConfig
     * \brief Configuration for the splash scene loaded from JSON
     */
    struct SplashSceneConfig : public Engine::IConfig
    {
        Json::Value layout;
        float fadeInDuration = 1.2f;
        float holdAndExpandDuration = 2.0f;
        float finalHoldDuration = 1.0f;

        float canvasRefWidth = 1600.0f;
        float canvasRefHeight = 900.0f;
        int canvasSortOrder = 100;

        int logoRenderOrder = 1;
        float logoExpandScale = 1.05f;

        float logoWidth = 600.0f;
        float logoHeight = 200.0f;
        std::string logoWhitePath = "Assets/UI/DigiPen_BLACK.png";

        float logo2Width = 600.0f;
        float logo2Height = 600.0f;
        std::string logo2Path = "Assets/Sprites/xav.jpg";

        std::string ccTextPath = "Assets/UI/DigiPen_CC_Text.png";
        float ccTextWidth = 1050.0f;
        float ccTextHeight = 50.0f;
        int ccTextRenderOrder = 1;
        float ccTextBottomMargin = 20.0f;

        float flickerDipDuration = 0.2f;
        float flickerDepth = 1.0f;
        float flickerDuration = 0.2f;
        float finalFlickerDuration = 0.2f;

        Engine::Color logoWhiteColor = Engine::Color(1.0f, 1.0f, 1.0f, 1.0f);
        Engine::Color logo2Color = Engine::Color(1.0f, 1.0f, 1.0f, 1.0f);
        Engine::Color backgroundColor = Engine::Color(0.0f, 0.0f, 0.0f, 1.0f);

        std::string nextSceneName = "MainMenu";

        /*!
         * \brief Parse a color object from JSON
         * \param j [in] JSON value
         * \param key [in] Key containing color object
         * \param fallback [in] Fallback color if key missing
         * \return Parsed color or fallback
         */
        static Engine::Color ParseColor(const Json::Value& j, const char* key, const Engine::Color& fallback)
        {
            if (!j.isMember(key) || !j[key].isObject()) return fallback;
            const auto& c = j[key];
            return Engine::Color(
                c.get("r", fallback.r).asFloat(),
                c.get("g", fallback.g).asFloat(),
                c.get("b", fallback.b).asFloat(),
                c.get("a", fallback.a).asFloat());
        }

        /*!
         * \brief Populate config fields from JSON
         * \param j [in] JSON node containing configuration
         */
        void FromJson(const Json::Value& j) override
        {
            layout = j["layout"];
            fadeInDuration = j.get("fadeInDuration", fadeInDuration).asFloat();
            holdAndExpandDuration = j.get("holdAndExpandDuration", holdAndExpandDuration).asFloat();
            finalHoldDuration = j.get("finalHoldDuration", finalHoldDuration).asFloat();

            canvasRefWidth = j.get("canvasRefWidth", canvasRefWidth).asFloat();
            canvasRefHeight = j.get("canvasRefHeight", canvasRefHeight).asFloat();
            canvasSortOrder = j.get("canvasSortOrder", canvasSortOrder).asInt();

            logoRenderOrder = j.get("logoRenderOrder", logoRenderOrder).asInt();
            logoExpandScale = j.get("logoExpandScale", logoExpandScale).asFloat();

            logoWidth = j.get("logoWidth", logoWidth).asFloat();
            logoHeight = j.get("logoHeight", logoHeight).asFloat();
            logoWhitePath = j.get("logoWhitePath", logoWhitePath).asString();

            logo2Width = j.get("logo2Width", logo2Width).asFloat();
            logo2Height = j.get("logo2Height", logo2Height).asFloat();
            logo2Path = j.get("logo2Path", logo2Path).asString();

            ccTextPath = j.get("ccTextPath", ccTextPath).asString();
            ccTextWidth = j.get("ccTextWidth", ccTextWidth).asFloat();
            ccTextHeight = j.get("ccTextHeight", ccTextHeight).asFloat();
            ccTextRenderOrder = j.get("ccTextRenderOrder", ccTextRenderOrder).asInt();
            ccTextBottomMargin = j.get("ccTextBottomMargin", ccTextBottomMargin).asFloat();

            flickerDipDuration = j.get("flickerDipDuration", flickerDipDuration).asFloat();
            flickerDepth = j.get("flickerDepth", flickerDepth).asFloat();
            flickerDuration = j.get("flickerDuration", flickerDuration).asFloat();
            finalFlickerDuration = j.get("finalFlickerDuration", finalFlickerDuration).asFloat();

            logoWhiteColor = ParseColor(j, "logoWhiteColor", logoWhiteColor);
            logo2Color = ParseColor(j, "logo2Color", logo2Color);
            backgroundColor = ParseColor(j, "backgroundColor", backgroundColor);

            nextSceneName = j.get("nextSceneName", nextSceneName).asString();
        }
    };

    SplashSceneConfig g_splashSceneConfig;
    bool g_splashSceneConfigLoaded = false;

    /*!
     * \brief Get singleton splash scene configuration (loads on first call)
     * \return Reference to loaded SplashSceneConfig
     */
    const SplashSceneConfig& GetSplashSceneConfig()
    {
        if (!g_splashSceneConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/splash_scene.json", g_splashSceneConfig);
            if (g_splashSceneConfig.fadeInDuration < 0.0f) g_splashSceneConfig.fadeInDuration = 0.0f;
            if (g_splashSceneConfig.holdAndExpandDuration < 0.0f) g_splashSceneConfig.holdAndExpandDuration = 0.0f;
            if (g_splashSceneConfig.finalHoldDuration < 0.0f) g_splashSceneConfig.finalHoldDuration = 0.0f;
            if (g_splashSceneConfig.flickerDipDuration < 0.0f) g_splashSceneConfig.flickerDipDuration = 0.0f;
            if (g_splashSceneConfig.flickerDuration < 0.0f) g_splashSceneConfig.flickerDuration = 0.0f;
            if (g_splashSceneConfig.finalFlickerDuration < 0.0f) g_splashSceneConfig.finalFlickerDuration = 0.0f;
            g_splashSceneConfigLoaded = true;
        }
        return g_splashSceneConfig;
    }
}

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
const char* SplashScene::LOGO2_PATH      = "Assets/Sprites/xav.jpg";

// Logo colors
Color SplashScene::LOGO_WHITE_COLOR = Color(1.0f, 1.0f, 1.0f, 1.0f);
Color SplashScene::LOGO2_COLOR      = Color(1.0f, 1.0f, 1.0f, 1.0f);

// CC text settings
const char* SplashScene::CC_TEXT_PATH          = "Assets/UI/DigiPen_CC_Text.png";
float SplashScene::CC_TEXT_WIDTH               = 1050.f;
float SplashScene::CC_TEXT_HEIGHT              = 50.0f;
int   SplashScene::CC_TEXT_RENDER_ORDER        = 1;
float SplashScene::CC_TEXT_BOTTOM_MARGIN       = 20.0f;

// Flicker settings
float SplashScene::FLICKER_DIP_DURATION   = 0.2f;
float SplashScene::FLICKER_DEPTH          = 1.f;
float SplashScene::FLICKER_DURATION       = 0.2f;
float SplashScene::FINAL_FLICKER_DURATION = 0.2f;

/*!
 * \brief Construct a SplashScene
 */
SplashScene::SplashScene() : Scene("SplashScene") {}

/*!
 * \brief Called when the scene is loaded; sets up UI and initial state
 */
void SplashScene::OnLoad() {
    const auto& cfg = GetSplashSceneConfig();
    FADE_IN_DURATION = cfg.fadeInDuration;
    HOLD_AND_EXPAND_DURATION = cfg.holdAndExpandDuration;
    FINAL_HOLD_DURATION = cfg.finalHoldDuration;
    CANVAS_REF_WIDTH = cfg.canvasRefWidth;
    CANVAS_REF_HEIGHT = cfg.canvasRefHeight;
    CANVAS_SORT_ORDER = cfg.canvasSortOrder;
    LOGO_RENDER_ORDER = cfg.logoRenderOrder;
    LOGO_EXPAND_SCALE = cfg.logoExpandScale;
    LOGO_WIDTH = cfg.logoWidth;
    LOGO_HEIGHT = cfg.logoHeight;
    LOGO_WHITE_PATH = cfg.logoWhitePath.c_str();
    LOGO2_WIDTH = cfg.logo2Width;
    LOGO2_HEIGHT = cfg.logo2Height;
    LOGO2_PATH = cfg.logo2Path.c_str();
    CC_TEXT_PATH = cfg.ccTextPath.c_str();
    CC_TEXT_WIDTH = cfg.ccTextWidth;
    CC_TEXT_HEIGHT = cfg.ccTextHeight;
    CC_TEXT_RENDER_ORDER = cfg.ccTextRenderOrder;
    CC_TEXT_BOTTOM_MARGIN = cfg.ccTextBottomMargin;
    LOGO_WHITE_COLOR = cfg.logoWhiteColor;
    LOGO2_COLOR = cfg.logo2Color;
    FLICKER_DIP_DURATION = cfg.flickerDipDuration;
    FLICKER_DEPTH = cfg.flickerDepth;
    FLICKER_DURATION = cfg.flickerDuration;
    FINAL_FLICKER_DURATION = cfg.finalFlickerDuration;

    Input::RegisterCursorUnlock("SplashScene");
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
    m_background->SetBackgroundColor(cfg.backgroundColor);

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
    m_logo->SetColor(Color(LOGO_WHITE_COLOR.r, LOGO_WHITE_COLOR.g, LOGO_WHITE_COLOR.b, 0.0f));
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

    // DigiPen CC text (bottom-center)
    m_ccText = m_canvas->CreateChild<Image>();
    m_ccText->name = "DigiPenCCText";
    m_ccText->SetAnchors({ 0.5f, 0.0f }, { 0.5f, 0.0f });  // bottom-center anchor
    m_ccText->SetPivot({ 0.5f, 0.0f });                     // pivot at bottom-center of element
    m_ccText->SetAnchoredPosition({ 0.0f, CC_TEXT_BOTTOM_MARGIN });
    m_ccText->SetSizeDelta({ CC_TEXT_WIDTH, CC_TEXT_HEIGHT });
    {
        Texture2D* tex = AssetManager::GetTexture(CC_TEXT_PATH);
        m_ccText->SetTexture(tex);
    }
    m_ccText->SetColor(Color(1.0f, 1.0f, 1.0f, 0.0f));
    m_ccText->SetRenderOrder(CC_TEXT_RENDER_ORDER);

    ApplyLayoutNode(m_background, cfg.layout);
    ApplyLayoutNode(m_logo, cfg.layout);
    ApplyLayoutNode(m_logo2, cfg.layout);
    ApplyLayoutNode(m_ccText, cfg.layout);

    // Start with screen fully black (fade overlay at alpha = 1)
    // Then fade it out to reveal our scene
    FadeService::FadeOut(FADE_IN_DURATION);
}

/*!
 * \brief Per-frame update handling splash phases and input
 */
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
        const_cast<Color&>(m_ccText->GetColor()).a = t;

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

/*!
 * \brief Advance the splash scene to the next phase
 */
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
		SceneManager::LoadWithFade(GetSplashSceneConfig().nextSceneName);
        break;

    case Phase::FadeOut:
        break;
    }
}

/*!
 * \brief Immediately start transition to the next scene (skip splash)
 */
void SplashScene::SkipToEnd() {
    if (m_transitioning) return;
    m_transitioning = true;

    SceneManager::LoadWithFade(GetSplashSceneConfig().nextSceneName);
}

/*!
 * \brief Query common inputs to determine if user requested skip
 * \return True if a skip key or mouse button was pressed
 */
/*!
 * \brief [AUTO] IsSkipPressed
 * \return 
 */
/*!
 * \brief [AUTO] IsSkipPressed
 * \return 
 */
/*!
 * \brief [AUTO] IsSkipPressed
 * \return 
 */
/*!
 * \brief [AUTO] IsSkipPressed
 * \return 
 */
/*!
 * \brief [AUTO] IsSkipPressed
 * \return 
 */
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

/*!
 * \brief Clean up UI and unregister input hooks when scene unloads
 */
void SplashScene::OnUnload() {
    Input::UnregisterCursorUnlock("SplashScene");
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
    m_ccText = nullptr;
}

} // namespace Game
