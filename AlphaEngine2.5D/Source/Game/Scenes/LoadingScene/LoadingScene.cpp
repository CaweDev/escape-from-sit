/**
 * @file LoadingScene.cpp
 * @brief Implementation of LoadingScene
 */
#include "LoadingScene.hpp"

namespace Game {

    LoadingScene::LoadingScene() : Scene("LoadingScene") {}

    void LoadingScene::OnLoad() {
        // Reset state
        m_elapsed = 0.0f;
        m_dotTimer = 0.0f;
        m_dotCount = 0;
        m_loadTriggered = false;

        auto* uiMgr = UIManager::GetInstance();
        if (!uiMgr) return;

        // ========== Canvas ==========
        m_canvas = uiMgr->CreateCanvas();
        m_canvas->SetReferenceResolution(1280.f, 720.f);
        m_canvas->SetScaleMode(CanvasScaleMode::ScaleWithScreenSize);
        m_canvas->SetSortOrder(100);

        // ========== Background ==========
        m_background = m_canvas->CreateChild<Panel>();
        m_background->name = "LoadingBg";
        m_background->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_background->SetAnchoredPosition({ 0.0f, 0.0f });
        m_background->SetSizeDelta({ 0.0f, 0.0f });
        m_background->SetBackgroundColor(Color(0.0f, 0.0f, 0.0f, 1.0f));

        // ========== Loading Text ==========
        m_loadingText = m_canvas->CreateChild<Text>();
        m_loadingText->name = "LoadingText";
        m_loadingText->SetAnchors({ 0.3f, 0.50f }, { 0.7f, 0.65f });
        m_loadingText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_loadingText->SetSizeDelta({ 0.0f, 0.0f });
        m_loadingText->SetText("LOADING");
        m_loadingText->SetFontSize(3.0f);
        m_loadingText->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
        m_loadingText->SetAnchor(TextAnchor::MiddleCenter);

        // ========== Progress Bar Background ==========
        m_progressBarBg = m_canvas->CreateChild<Image>();
        m_progressBarBg->name = "ProgressBarBg";
        m_progressBarBg->SetAnchors({ 0.3f, 0.40f }, { 0.7f, 0.40f });
        m_progressBarBg->SetPivot({ 0.5f, 0.5f });
        m_progressBarBg->SetAnchoredPosition({ 0.0f, 0.0f });
        m_progressBarBg->SetSizeDelta({ 0.0f, 8.0f });
        m_progressBarBg->SetColor(Color(0.2f, 0.2f, 0.2f, 1.0f));

        // ========== Progress Bar Fill ==========
        m_progressBarFill = m_canvas->CreateChild<Image>();
        m_progressBarFill->name = "ProgressBarFill";
        m_progressBarFill->SetAnchors({ 0.3f, 0.40f }, { 0.7f, 0.40f });
        m_progressBarFill->SetPivot({ 0.5f, 0.5f });
        m_progressBarFill->SetAnchoredPosition({ 0.0f, 0.0f });
        m_progressBarFill->SetSizeDelta({ 0.0f, 8.0f });
        m_progressBarFill->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
        m_progressBarFill->SetFillAmount(0.0f);
        m_progressBarFill->SetRenderOrder(1);

        // Fade OUT the overlay to reveal the loading UI
        // (The overlay is at alpha=1 from the previous FadeIn)
        UI::FadeService::FadeOut(SceneManager::GetLoadWithScreenFadeInDuration());
    }

    void LoadingScene::OnUpdate() {
        float dt = Time::unscaledDeltaTime;
        m_elapsed += dt;

        // Animate loading dots
        m_dotTimer += dt;
        if (m_dotTimer >= DOT_INTERVAL) {
            m_dotTimer -= DOT_INTERVAL;
            m_dotCount = (m_dotCount + 1) % 4;

            std::string dots(m_dotCount, '.');
            m_loadingText->SetText("LOADING" + dots);
        }

        // Animate progress bar
        float progress = m_elapsed / PROGRESS_DURATION;
        if (progress > 1.0f) progress = 1.0f;
        m_progressBarFill->SetFillAmount(progress);

        // After minimum display time, fade to black then load target
        if (!m_loadTriggered && m_elapsed >= MIN_DISPLAY_TIME) {
            m_loadTriggered = true;

            std::string target = SceneManager::GetLoadWithScreenTarget();
            f32 fadeOutDuration = SceneManager::GetLoadWithScreenFadeOutDuration();


            FadeService::FadeIn(fadeOutDuration, [target, fadeOutDuration]() {
                // Screen is now fully black — load target and start reveal
                SceneManager::LoadSceneDeferred(target);
                FadeService::FadeOut(fadeOutDuration);
            });
        }
    }

    void LoadingScene::OnUnload() {
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
    }

} // namespace Game
