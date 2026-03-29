/**
 * @file LoadingScene.cpp
 * @brief Implementation of LoadingScene
 */
#include "LoadingScene.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include "Engine/Input/Input.hpp"
#include <random>

namespace {

    struct LoadingSceneConfig : public Engine::IConfig {
        float referenceWidth = 1280.0f;
        float referenceHeight = 720.0f;
        int canvasSortOrder = 100;

        float minDisplayTime = 1.2f;
        float progressDuration = 1.2f;
        float dotInterval = 0.3f;

        std::string tipsConfigPath = "Assets/Config/LoadingScene/tips.json";

        float loadingFontSize = 3.0f;
        float tipFontSize = 1.5f;
        float progressBarHeight = 8.0f;

        Engine::Color backgroundColor = Engine::Color(0.0f, 0.0f, 0.0f, 1.0f);
        Engine::Color loadingTextColor = Engine::Color(1.0f, 1.0f, 1.0f, 1.0f);
        Engine::Color progressBarBgColor = Engine::Color(0.2f, 0.2f, 0.2f, 1.0f);
        Engine::Color progressBarFillColor = Engine::Color(1.0f, 1.0f, 1.0f, 1.0f);
        Engine::Color tipTextColor = Engine::Color(0.8f, 0.8f, 0.8f, 1.0f);

        static Engine::Color ParseColor(const Json::Value& j, const char* key, const Engine::Color& fallback)
        {
            if (!j.isMember(key) || !j[key].isObject()) return fallback;
            const Json::Value& c = j[key];
            return Engine::Color(
                c.get("r", fallback.r).asFloat(),
                c.get("g", fallback.g).asFloat(),
                c.get("b", fallback.b).asFloat(),
                c.get("a", fallback.a).asFloat());
        }

        void FromJson(const Json::Value& j) override {
            referenceWidth = j.get("referenceWidth", referenceWidth).asFloat();
            referenceHeight = j.get("referenceHeight", referenceHeight).asFloat();
            canvasSortOrder = j.get("canvasSortOrder", canvasSortOrder).asInt();

            minDisplayTime = j.get("minDisplayTime", minDisplayTime).asFloat();
            progressDuration = j.get("progressDuration", progressDuration).asFloat();
            dotInterval = j.get("dotInterval", dotInterval).asFloat();

            tipsConfigPath = j.get("tipsConfigPath", tipsConfigPath).asString();

            loadingFontSize = j.get("loadingFontSize", loadingFontSize).asFloat();
            tipFontSize = j.get("tipFontSize", tipFontSize).asFloat();
            progressBarHeight = j.get("progressBarHeight", progressBarHeight).asFloat();

            backgroundColor = ParseColor(j, "backgroundColor", backgroundColor);
            loadingTextColor = ParseColor(j, "loadingTextColor", loadingTextColor);
            progressBarBgColor = ParseColor(j, "progressBarBgColor", progressBarBgColor);
            progressBarFillColor = ParseColor(j, "progressBarFillColor", progressBarFillColor);
            tipTextColor = ParseColor(j, "tipTextColor", tipTextColor);
        }
    };

    LoadingSceneConfig g_loadingSceneConfig;
    bool g_loadingSceneConfigLoaded = false;

    const LoadingSceneConfig& GetLoadingSceneConfig()
    {
        if (!g_loadingSceneConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/loading_scene.json", g_loadingSceneConfig);
            if (g_loadingSceneConfig.referenceWidth < 1.0f) g_loadingSceneConfig.referenceWidth = 1.0f;
            if (g_loadingSceneConfig.referenceHeight < 1.0f) g_loadingSceneConfig.referenceHeight = 1.0f;
            if (g_loadingSceneConfig.minDisplayTime < 0.0f) g_loadingSceneConfig.minDisplayTime = 0.0f;
            if (g_loadingSceneConfig.progressDuration < 0.01f) g_loadingSceneConfig.progressDuration = 0.01f;
            if (g_loadingSceneConfig.dotInterval < 0.01f) g_loadingSceneConfig.dotInterval = 0.01f;
            if (g_loadingSceneConfig.progressBarHeight < 0.1f) g_loadingSceneConfig.progressBarHeight = 0.1f;
            g_loadingSceneConfigLoaded = true;
        }
        return g_loadingSceneConfig;
    }

}

namespace Game {

    LoadingScene::LoadingScene() : Scene("LoadingScene") {}

    /// <summary>
	/// This struct is to read from Assets/Config/tips.json using ConfigManager
    /// </summary>
    struct TipsConfig : public Engine::IConfig {
        std::vector<std::string> tips;

        void FromJson(const Json::Value& j) override {
            tips.clear();
            const Json::Value& tipsArray = j["tips"];
            if (tipsArray.isArray()) {
                for (const auto& tip : tipsArray) {
                    tips.push_back(tip.asString());
                }
            }
        }
    };

    /*!
     * \brief [AUTO] OnLoad
     */
    void LoadingScene::OnLoad() {
        const auto& cfg = GetLoadingSceneConfig();
        Input::RegisterCursorUnlock("LoadingScene");
        // Reset state
        m_elapsed = 0.0f;
        m_dotTimer = 0.0f;
        m_dotCount = 0;
        m_loadTriggered = false;

        auto* uiMgr = UIManager::GetInstance();
        if (!uiMgr) return;

        // ========== Canvas ==========
        m_canvas = uiMgr->CreateCanvas();
        m_canvas->SetReferenceResolution(cfg.referenceWidth, cfg.referenceHeight);
        m_canvas->SetScaleMode(CanvasScaleMode::ScaleWithScreenSize);
        m_canvas->SetSortOrder(cfg.canvasSortOrder);

        // ========== Background ==========
        m_background = m_canvas->CreateChild<Panel>();
        m_background->name = "LoadingBg";
        m_background->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_background->SetAnchoredPosition({ 0.0f, 0.0f });
        m_background->SetSizeDelta({ 0.0f, 0.0f });
        m_background->SetBackgroundColor(cfg.backgroundColor);

        // ========== Loading Text ==========
        m_loadingText = m_canvas->CreateChild<Text>();
        m_loadingText->name = "LoadingText";
        m_loadingText->SetAnchors({ 0.3f, 0.50f }, { 0.7f, 0.65f });
        m_loadingText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_loadingText->SetSizeDelta({ 0.0f, 0.0f });
        m_loadingText->SetText("LOADING");
        m_loadingText->SetFontSize(cfg.loadingFontSize);
        m_loadingText->SetColor(cfg.loadingTextColor);
        m_loadingText->SetAnchor(TextAnchor::MiddleCenter);

        // ========== Progress Bar Background ==========
        m_progressBarBg = m_canvas->CreateChild<Image>();
        m_progressBarBg->name = "ProgressBarBg";
        m_progressBarBg->SetAnchors({ 0.3f, 0.40f }, { 0.7f, 0.40f });
        m_progressBarBg->SetPivot({ 0.5f, 0.5f });
        m_progressBarBg->SetAnchoredPosition({ 0.0f, 0.0f });
        m_progressBarBg->SetSizeDelta({ 0.0f, cfg.progressBarHeight });
        m_progressBarBg->SetColor(cfg.progressBarBgColor);

        // ========== Progress Bar Fill ==========
        m_progressBarFill = m_canvas->CreateChild<Image>();
        m_progressBarFill->name = "ProgressBarFill";
        m_progressBarFill->SetAnchors({ 0.3f, 0.40f }, { 0.7f, 0.40f });
        m_progressBarFill->SetPivot({ 0.5f, 0.5f });
        m_progressBarFill->SetAnchoredPosition({ 0.0f, 0.0f });
        m_progressBarFill->SetSizeDelta({ 0.0f, cfg.progressBarHeight });
        m_progressBarFill->SetColor(cfg.progressBarFillColor);
        m_progressBarFill->SetFillAmount(0.0f);
        m_progressBarFill->SetRenderOrder(1);

        // ========== Tip Text ==========
        m_tipText = m_canvas->CreateChild<Text>();
        m_tipText->name = "TipText";
        m_tipText->SetAnchors({ 0.1f, 0.20f }, { 0.9f, 0.35f });
        m_tipText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_tipText->SetSizeDelta({ 0.0f, 0.0f });
        m_tipText->SetFontSize(cfg.tipFontSize);
        m_tipText->SetColor(cfg.tipTextColor);
        m_tipText->SetAnchor(TextAnchor::MiddleCenter);

        // Load tips from config if not already loaded
        if (m_tips.empty()) {
            TipsConfig config;
            if (Engine::ConfigManager::ReadConfig(cfg.tipsConfigPath, config)) {
                m_tips = config.tips;
            }
        }

        // Pick a random tip
        if (!m_tips.empty()) {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<size_t> distrib(0, m_tips.size() - 1);
            m_tipText->SetText(m_tips[distrib(gen)]);
        } else {
            m_tipText->SetText("");
        }

        // Fade OUT the overlay to reveal the loading UI
        // (The overlay is at alpha=1 from the previous FadeIn)
        UI::FadeService::FadeOut(SceneManager::GetLoadWithScreenFadeInDuration());
    }

    /*!
     * \brief [AUTO] OnUpdate
     */
    void LoadingScene::OnUpdate() {
        const auto& cfg = GetLoadingSceneConfig();
        float dt = Time::unscaledDeltaTime;
        m_elapsed += dt;

        // Animate loading dots
        m_dotTimer += dt;
        if (m_dotTimer >= cfg.dotInterval) {
            m_dotTimer -= cfg.dotInterval;
            m_dotCount = (m_dotCount + 1) % 4;

            std::string dots(m_dotCount, '.');
            m_loadingText->SetText("LOADING" + dots);
        }

        // Animate progress bar
        float progress = m_elapsed / cfg.progressDuration;
        if (progress > 1.0f) progress = 1.0f;
        m_progressBarFill->SetFillAmount(progress);

        // After minimum display time, hide loading widgets, then switch and reveal
        if (!m_loadTriggered && m_elapsed >= cfg.minDisplayTime) {
            m_loadTriggered = true;

            std::string target = SceneManager::GetLoadWithScreenTarget();
            f32 fadeOutDuration = SceneManager::GetLoadWithScreenFadeOutDuration();

            if (m_loadingText)    m_loadingText->visible = false;
            if (m_progressBarBg)  m_progressBarBg->visible = false;
            if (m_progressBarFill)m_progressBarFill->visible = false;
            if (m_tipText)        m_tipText->visible = false;

          /*!
           * \brief [AUTO] FadeIn
           * \param 0.0f [in]
           * \param [target [in]
           * \param fadeOutDuration]( [in]
           */
            FadeService::FadeIn(0.0f, [target, fadeOutDuration]() {
                SceneManager::RequestFadeOutAfterNextSceneLoad(fadeOutDuration);
                SceneManager::LoadScene(target);
            });
        }
    }

    /*!
     * \brief [AUTO] OnUnload
     */
    void LoadingScene::OnUnload() {
        Input::UnregisterCursorUnlock("LoadingScene");
        if (m_canvas) {
            auto* uiMgr = UIManager::GetInstance();
            if (uiMgr) {
                uiMgr->DestroyCanvas(m_canvas);
            }
            m_canvas = nullptr;
        }
        m_background = nullptr;
        m_loadingText = nullptr;
        m_progressBarBg = nullptr;
        m_progressBarFill = nullptr;
        m_tipText = nullptr;
    }

} // namespace Game
