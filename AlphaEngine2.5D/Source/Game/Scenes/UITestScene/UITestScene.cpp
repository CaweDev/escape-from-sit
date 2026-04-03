/**
 * @file UITestScene.cpp
 * @brief Implementation of the UI test scene
 */

#include "Engine.hpp"
#include "Engine/Core/ConfigManager.hpp"

#include "UITestScene.hpp"

namespace
{
    struct UITestSceneConfig : public Engine::IConfig
    {
        float referenceWidth = 1280.0f;
        float referenceHeight = 720.0f;
        float panelWidth = 400.0f;
        float panelHeight = 600.0f;
        Engine::Color panelColor = Engine::Color(0.1f, 0.1f, 0.1f, 0.8f);
        std::string panelTexturePath = "Assets/Tilemap/modern_school_floor.png";

        float titleFontSize = 2.5f;
        Engine::Color titleColor = Engine::Color::white();

        float menuSpacing = 15.0f;
        float menuHorizontalInset = -40.0f;
        float menuButtonHeight = 60.0f;
        Engine::Color menuButtonColor = Engine::Color(0.3f, 0.3f, 0.5f, 1.0f);

        float badgeSpacing = 20.0f;
        float badgeSize = 40.0f;
        float panelDriftPerFrame = 1.0f;

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
            referenceWidth = j.get("referenceWidth", referenceWidth).asFloat();
            referenceHeight = j.get("referenceHeight", referenceHeight).asFloat();
            panelWidth = j.get("panelWidth", panelWidth).asFloat();
            panelHeight = j.get("panelHeight", panelHeight).asFloat();
            panelColor = ParseColor(j, "panelColor", panelColor);
            panelTexturePath = j.get("panelTexturePath", panelTexturePath).asString();

            titleFontSize = j.get("titleFontSize", titleFontSize).asFloat();
            titleColor = ParseColor(j, "titleColor", titleColor);

            menuSpacing = j.get("menuSpacing", menuSpacing).asFloat();
            menuHorizontalInset = j.get("menuHorizontalInset", menuHorizontalInset).asFloat();
            menuButtonHeight = j.get("menuButtonHeight", menuButtonHeight).asFloat();
            menuButtonColor = ParseColor(j, "menuButtonColor", menuButtonColor);

            badgeSpacing = j.get("badgeSpacing", badgeSpacing).asFloat();
            badgeSize = j.get("badgeSize", badgeSize).asFloat();
            panelDriftPerFrame = j.get("panelDriftPerFrame", panelDriftPerFrame).asFloat();
        }
    };

    UITestSceneConfig g_uiTestSceneConfig;
    bool g_uiTestSceneConfigLoaded = false;

    const UITestSceneConfig& GetUITestSceneConfig()
    {
        if (!g_uiTestSceneConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/ui_test_scene.json", g_uiTestSceneConfig);
            g_uiTestSceneConfigLoaded = true;
        }
        return g_uiTestSceneConfig;
    }
}

namespace Game {

UITestScene::UITestScene() : Scene("UITest") {}
UITestScene::~UITestScene() {}


void UITestScene::OnLoad() {
    const auto& cfg = GetUITestSceneConfig();
    Input::RegisterCursorUnlock("UITestScene");
    
    // Create the main canvas
    mainCanvas = UIManager::GetInstance()->CreateCanvas();
    mainCanvas->SetReferenceResolution(cfg.referenceWidth, cfg.referenceHeight);
    mainCanvas->SetScaleMode(CanvasScaleMode::ScaleWithScreenSize);

    // 1. Create a semi-transparent background panel in the center
    panel = mainCanvas->CreateChild<Panel>();
    panel->name = "MainPanel";
    panel->rectTransform.SetCentered(cfg.panelWidth, cfg.panelHeight);
    panel->SetBackgroundColor(cfg.panelColor);
	panel->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.panelTexturePath.c_str()));


    // 2. Add a title text at the top of the panel
    Text* title = panel->CreateChild<Text>();
    title->name = "TitleText";
    title->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
    title->SetPivot({ 0.5f, 1.0f });
    title->SetAnchoredPosition({ 0.0f, -20.0f });
    title->SetSizeDelta({ 0.0f, 60.0f });
    title->SetText("UI Test Scene");
    title->SetFontSize(cfg.titleFontSize);
    title->SetColor(cfg.titleColor);
    title->SetAnchor(TextAnchor::MiddleCenter);

    // 3. Create a vertical layout group for the main menu buttons
    VerticalLayoutGroup* vlg = panel->CreateChild<VerticalLayoutGroup>();
    vlg->name = "MenuLayout";
    vlg->SetAnchors({ 0.0f, 0.3f }, { 1.0f, 0.8f });
    vlg->SetAnchoredPosition({ 0.0f, 0.0f });
    vlg->SetSizeDelta({ cfg.menuHorizontalInset, 0.0f }); // Horizontal padding
    vlg->SetSpacing(cfg.menuSpacing);
    vlg->SetChildAlignment(ChildAlignment::UpperCenter);
    vlg->SetChildForceExpandWidth(true);

    // Add buttons to the vertical layout
    const char* buttonNames[] = { "Start Game", "Options", "Exit" };
    for (int i = 0; i < 3; ++i) {
        Button* btn = vlg->CreateChild<Button>();
        btn->name = buttonNames[i];
        btn->SetSizeDelta({ 0.0f, cfg.menuButtonHeight });
        btn->SetText(buttonNames[i]);
        btn->SetBackgroundColor(cfg.menuButtonColor);
        
        btn->onClick = []() {
        };
    }

    // 4. Add a horizontal layout group at the bottom for some "badges"
    HorizontalLayoutGroup* hlg = panel->CreateChild<HorizontalLayoutGroup>();
    hlg->name = "BadgeLayout";
    hlg->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 0.2f });
    hlg->SetAnchoredPosition({ 0.0f, 20.0f });
    hlg->SetSizeDelta({ -40.0f, 0.0f });
    hlg->SetSpacing(cfg.badgeSpacing);
    hlg->SetChildAlignment(ChildAlignment::MiddleCenter);

    // Add some color badges
    Engine::Color badgeColors[] = { Engine::Color::red(), Engine::Color::green(), Engine::Color::blue() };
    for (int i = 0; i < 3; ++i) {
        Image* badge = hlg->CreateChild<Image>();
        badge->name = "Badge";
        badge->SetSizeDelta({ cfg.badgeSize, cfg.badgeSize });
        badge->SetColor(badgeColors[i]);
    }
}


void UITestScene::OnUnload() {
    Input::UnregisterCursorUnlock("UITestScene");
    // UI System shutdown is now handled globally in Main.cpp
}


void UITestScene::OnUpdate() {
    const auto& cfg = GetUITestSceneConfig();
    panel->SetAnchoredPosition({ panel->GetAnchoredPosition().x, panel->GetAnchoredPosition().y + cfg.panelDriftPerFrame});
}

} // namespace Game
