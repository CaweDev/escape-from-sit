#include "CreditScene.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/UI/Core/UIManager.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/AssetManager.hpp"
#include "Engine/Graphics/Color.hpp"
#include "Engine/Input/Input.hpp"
#include <iostream>

namespace Game {

float CreditScene::CANVAS_REF_WIDTH  = 1600.f;
float CreditScene::CANVAS_REF_HEIGHT = 900.f;
const char* CreditScene::CREDITS_TEXTURE_PATH = "Assets/UI/CreditScene.png";

CreditScene::CreditScene() : Scene("CreditScene") {}

void CreditScene::OnLoad() {
    m_transitioning = false;
    
    // Canvas setup
    m_canvas = UIManager::GetInstance()->CreateCanvas();
    m_canvas->SetReferenceResolution(CANVAS_REF_WIDTH, CANVAS_REF_HEIGHT);
    m_canvas->SetScaleMode(CanvasScaleMode::ScaleWithScreenSize);
    m_canvas->SetSortOrder(100);

    // Black background
    m_background = m_canvas->CreateChild<Panel>();
    m_background->name = "CreditBg";
    m_background->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
    m_background->SetAnchoredPosition({ 0.0f, 0.0f });
    m_background->SetSizeDelta({ 0.0f, 0.0f });
    m_background->SetBackgroundColor(Color(0.0f, 0.0f, 0.0f, 1.0f));

    // Credits image
    m_creditsImage = m_canvas->CreateChild<Image>();
    m_creditsImage->name = "CreditsImage";
    m_creditsImage->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
    m_creditsImage->SetPivot({ 0.5f, 0.5f });

    Texture2D* tex = AssetManager::GetTexture(CREDITS_TEXTURE_PATH);
    if (tex) {
        m_creditsImage->SetTexture(tex);
        
        // Calculate size: width is canvas width, height scales proportionally 
        float imageWidth = CANVAS_REF_WIDTH;
        float imageHeight = (CANVAS_REF_WIDTH / (float)tex->GetPixelWidth()) * (float)tex->GetPixelHeight();
        
        m_creditsImage->SetSizeDelta({ imageWidth, imageHeight });

        // Start with top of image near top of screen (or slightly below to fade in nicely)
        // Image at Y=0 means its center is at screen center.
        // Image top is at Y = imageHeight / 2. Screen top is CANVAS_REF_HEIGHT / 2.
        float startY = -(imageHeight / 2.0f - CANVAS_REF_HEIGHT / 2.0f);
        m_currentY = startY;
        
        // End when bottom of image passes bottom of screen
        m_targetY = (imageHeight / 2.0f - CANVAS_REF_HEIGHT / 2.0f) + 100.0f; // Let it scroll a bit past
        
        m_creditsImage->SetAnchoredPosition({ 0.0f, m_currentY });
    }
    m_creditsImage->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
    m_creditsImage->SetRenderOrder(1);

    FadeService::FadeOut(1.0f);
}

void CreditScene::OnUpdate() {
    float dt = Time::unscaledDeltaTime;

    if (!m_transitioning) {
        if (IsSkipPressed()) {
            SkipToEnd();
            return;
        }

        // Scroll the image upwards
        m_currentY += m_scrollSpeed * dt;
        m_creditsImage->SetAnchoredPosition({ 0.0f, m_currentY });

        if (m_currentY > m_targetY) {
            SkipToEnd();
        }
    }
}

void CreditScene::SkipToEnd() {
    if (m_transitioning) return;
    m_transitioning = true;
    SceneManager::LoadWithFade("MainMenu");
}

bool CreditScene::IsSkipPressed() const {
    if (Input::GetKeyDown(KeyCode::Return) ||
        Input::GetKeyDown(KeyCode::Escape) ||
        Input::GetKeyDown(KeyCode::Space) ||
        Input::GetMouseButtonDown(0) ||
        Input::GetMouseButtonDown(1)) {
        return true;
    }
    return false;
}

void CreditScene::OnUnload() {
    if (m_canvas) {
        auto* uiMgr = UIManager::GetInstance();
        if (uiMgr) {
            uiMgr->DestroyCanvas(m_canvas);
        }
        m_canvas = nullptr;
    }
    m_background = nullptr;
    m_creditsImage = nullptr;
}

} // namespace Game
