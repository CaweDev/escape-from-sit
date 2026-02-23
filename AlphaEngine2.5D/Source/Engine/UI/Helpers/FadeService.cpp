/**
 * @file FadeService.cpp
 * @brief FadeService implementation — screen fade transitions
 */

#include "Engine/UI/Helpers/FadeService.hpp"
#include "Engine/UI/Core/UIManager.hpp"
#include "Engine/UI/Core/Canvas.hpp"
#include "Engine/UI/Components/Image.hpp"
#include "Engine/Graphics/Color.hpp"

namespace UI
{
    // Static member definitions
    Canvas* FadeService::s_fadeCanvas = nullptr;
    Image*  FadeService::s_fadeImage  = nullptr;

    bool  FadeService::s_fading      = false;
    f32   FadeService::s_fadeAlpha    = 0.0f;
    f32   FadeService::s_fadeFrom     = 0.0f;
    f32   FadeService::s_fadeTo       = 0.0f;
    f32   FadeService::s_fadeDuration = 0.0f;
    f32   FadeService::s_fadeElapsed  = 0.0f;
    std::function<void()> FadeService::s_fadeCallback;

    void FadeService::Initialize()
    {
        if (s_fadeCanvas) return;  // Already initialized

        UIManager* mgr = UIManager::GetInstance();
        if (!mgr) return;

        // Dedicated canvas at the highest sort order
        s_fadeCanvas = mgr->CreateCanvas();
        s_fadeCanvas->SetSortOrder(9999);
        s_fadeCanvas->SetReferenceResolution(1280.f, 720.f);
        s_fadeCanvas->SetScaleMode(CanvasScaleMode::ScaleWithScreenSize);

        s_fadeImage = s_fadeCanvas->CreateChild<Image>();
        s_fadeImage->name = "FadeOverlay";
        s_fadeImage->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        s_fadeImage->SetAnchoredPosition({ 0.0f, 0.0f });
        s_fadeImage->SetSizeDelta({ 0.0f, 0.0f });
        s_fadeImage->SetColor(Engine::Color(0.0f, 0.0f, 0.0f, 0.0f));
        s_fadeImage->raycastTarget = false;
        s_fadeImage->visible = false;
    }

    void FadeService::Shutdown()
    {
        // Canvas is owned by UIManager and will be destroyed there.
        // Just clear our references.
        s_fadeCanvas = nullptr;
        s_fadeImage = nullptr;
        s_fading = false;
        s_fadeCallback = nullptr;
    }

    void FadeService::Update(f32 dt)
    {
        if (!s_fading || !s_fadeImage) return;

        s_fadeElapsed += dt;
        f32 t = (s_fadeDuration > 0.0f) ? (s_fadeElapsed / s_fadeDuration) : 1.0f;

        if (t >= 1.0f)
        {
            t = 1.0f;
            s_fading = false;
            s_fadeAlpha = s_fadeTo;
            s_fadeImage->SetColor(Engine::Color(0.0f, 0.0f, 0.0f, s_fadeAlpha));

            // Hide overlay when fully transparent
            if (s_fadeAlpha <= 0.0f)
            {
                s_fadeImage->visible = false;
            }

            // Fire callback
            if (s_fadeCallback)
            {
                auto cb = s_fadeCallback;
                s_fadeCallback = nullptr;
                cb();
            }
            return;
        }

        // Smooth interpolation (smoothstep)
        s_fadeAlpha = s_fadeFrom + (s_fadeTo - s_fadeFrom) * (t * t * (3.0f - 2.0f * t));
        s_fadeImage->SetColor(Engine::Color(0.0f, 0.0f, 0.0f, s_fadeAlpha));
    }

    void FadeService::FadeIn(f32 duration, std::function<void()> onComplete)
    {
        if (!s_fadeImage) return;

        s_fadeImage->visible = true;
        s_fadeFrom = 0.0f;
        s_fadeTo = 1.0f;
        s_fadeDuration = duration;
        s_fadeElapsed = 0.0f;
        s_fadeAlpha = 0.0f;
        s_fadeCallback = onComplete;
        s_fading = true;

        s_fadeImage->SetColor(Engine::Color(0.0f, 0.0f, 0.0f, 0.0f));
    }

    void FadeService::FadeOut(f32 duration, std::function<void()> onComplete)
    {
        if (!s_fadeImage) return;

        s_fadeImage->visible = true;
        s_fadeFrom = 1.0f;
        s_fadeTo = 0.0f;
        s_fadeDuration = duration;
        s_fadeElapsed = 0.0f;
        s_fadeAlpha = 1.0f;
        s_fadeCallback = onComplete;
        s_fading = true;

        s_fadeImage->SetColor(Engine::Color(0.0f, 0.0f, 0.0f, 1.0f));
    }

} // namespace UI
