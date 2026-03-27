#include "CreditScene.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/UI/Core/UIManager.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/AssetManager.hpp"
#include "Engine/Graphics/Color.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <iostream>

namespace
{
    bool TryReadVec2(const Json::Value& node, const char* key, AEVec2& out)
    {
        if (!node.isMember(key) || !node[key].isObject()) return false;
        const auto& v = node[key];
        out.x = v.get("x", out.x).asFloat();
        out.y = v.get("y", out.y).asFloat();
        return true;
    }

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

    struct CreditSceneConfig : public Engine::IConfig
    {
        Json::Value layout;
        float canvasRefWidth = 1600.0f;
        float canvasRefHeight = 900.0f;
        int canvasSortOrder = 100;
        std::string creditsTexturePath = "Assets/UI/CreditScene.png";
        float imageScrollSpeed = 100.0f;
        float imageEndPadding = 100.0f;
        float fadeOutDuration = 1.0f;
        Engine::Color backgroundColor = Engine::Color(0.0f, 0.0f, 0.0f, 1.0f);
        Engine::Color creditsTint = Engine::Color(1.0f, 1.0f, 1.0f, 1.0f);
        std::string creditsBgmKey = "credits";

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

        void FromJson(const Json::Value& j) override
        {
            layout = j["layout"];
            canvasRefWidth = j.get("canvasRefWidth", canvasRefWidth).asFloat();
            canvasRefHeight = j.get("canvasRefHeight", canvasRefHeight).asFloat();
            canvasSortOrder = j.get("canvasSortOrder", canvasSortOrder).asInt();
            creditsTexturePath = j.get("creditsTexturePath", creditsTexturePath).asString();
            imageScrollSpeed = j.get("imageScrollSpeed", imageScrollSpeed).asFloat();
            imageEndPadding = j.get("imageEndPadding", imageEndPadding).asFloat();
            fadeOutDuration = j.get("fadeOutDuration", fadeOutDuration).asFloat();
            backgroundColor = ParseColor(j, "backgroundColor", backgroundColor);
            creditsTint = ParseColor(j, "creditsTint", creditsTint);
            creditsBgmKey = j.get("creditsBgmKey", creditsBgmKey).asString();
        }
    };

    CreditSceneConfig g_creditSceneConfig;
    bool g_creditSceneConfigLoaded = false;

    const CreditSceneConfig& GetCreditSceneConfig()
    {
        if (!g_creditSceneConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/credit_scene.json", g_creditSceneConfig);
            if (g_creditSceneConfig.canvasRefWidth < 1.0f) g_creditSceneConfig.canvasRefWidth = 1.0f;
            if (g_creditSceneConfig.canvasRefHeight < 1.0f) g_creditSceneConfig.canvasRefHeight = 1.0f;
            g_creditSceneConfigLoaded = true;
        }
        return g_creditSceneConfig;
    }
}

namespace Game {

float CreditScene::CANVAS_REF_WIDTH  = 1600.f;
float CreditScene::CANVAS_REF_HEIGHT = 900.f;
const char* CreditScene::CREDITS_TEXTURE_PATH = "Assets/UI/CreditScene.png";

CreditScene::CreditScene() : Scene("CreditScene") {}

void CreditScene::OnLoad() {
    const auto& cfg = GetCreditSceneConfig();
    Input::RegisterCursorUnlock("CreditScene");
    SoundManager::PlayBGM(cfg.creditsBgmKey, true);
    m_transitioning = false;

    CANVAS_REF_WIDTH = cfg.canvasRefWidth;
    CANVAS_REF_HEIGHT = cfg.canvasRefHeight;
    m_scrollSpeed = cfg.imageScrollSpeed;

    // Canvas setup
    m_canvas = UIManager::GetInstance()->CreateCanvas();
    m_canvas->SetReferenceResolution(CANVAS_REF_WIDTH, CANVAS_REF_HEIGHT);
    m_canvas->SetScaleMode(CanvasScaleMode::ScaleWithScreenSize);
    m_canvas->SetSortOrder(cfg.canvasSortOrder);

    // Black background
    m_background = m_canvas->CreateChild<Panel>();
    m_background->name = "CreditBg";
    m_background->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
    m_background->SetAnchoredPosition({ 0.0f, 0.0f });
    m_background->SetSizeDelta({ 0.0f, 0.0f });
    m_background->SetBackgroundColor(cfg.backgroundColor);

    // Credits image
    m_creditsImage = m_canvas->CreateChild<Image>();
    m_creditsImage->name = "CreditsImage";
    m_creditsImage->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
    m_creditsImage->SetPivot({ 0.5f, 0.5f });

    Texture2D* tex = AssetManager::GetTexture(cfg.creditsTexturePath.c_str());
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
        m_targetY = (imageHeight / 2.0f - CANVAS_REF_HEIGHT / 2.0f) + cfg.imageEndPadding;
        
        m_creditsImage->SetAnchoredPosition({ 0.0f, m_currentY });
    }
    m_creditsImage->SetColor(cfg.creditsTint);
    m_creditsImage->SetRenderOrder(1);

    ApplyLayoutNode(m_background, cfg.layout);
    ApplyLayoutNode(m_creditsImage, cfg.layout);

    FadeService::FadeOut(cfg.fadeOutDuration);
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
    Input::UnregisterCursorUnlock("CreditScene");
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
