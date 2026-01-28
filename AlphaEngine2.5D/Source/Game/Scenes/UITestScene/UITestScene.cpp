/**
 * @file UITestScene.cpp
 * @brief Implementation of the UI test scene
 */

#include <iostream>
#include "Engine.hpp"

#include "UITestScene.hpp"

namespace Game {

UITestScene::UITestScene() : Scene("UITest") {}
UITestScene::~UITestScene() {}

void UITestScene::OnLoad() {
    // UI System is now initialized globally in Main.cpp
    
    // Create the main canvas
    mainCanvas = UIManager::GetInstance()->CreateCanvas();
    mainCanvas->SetReferenceResolution(1280.f, 720.f);
    mainCanvas->SetScaleMode(CanvasScaleMode::ScaleWithScreenSize);

    // 1. Create a semi-transparent background panel in the center
    panel = mainCanvas->CreateChild<Panel>();
    panel->name = "MainPanel";
    panel->rectTransform.SetCentered(400, 600);
    panel->SetBackgroundColor(Engine::Color(0.1f, 0.1f, 0.1f, 0.8f));
    //auto skySprite = new Texture2D();
    //skySprite->Load("Assets/Tilemap/modern_school_floor.png");
    //panel->SetBackgroundSprite(skySprite);
	panel->SetBackgroundSprite(Engine::AssetManager::GetTexture("Assets/Tilemap/modern_school_floor.png"));


    // 2. Add a title text at the top of the panel
    Text* title = panel->CreateChild<Text>();
    title->name = "TitleText";
    title->rectTransform.anchorMin = { 0.0f, 1.0f };
    title->rectTransform.anchorMax = { 1.0f, 1.0f };
    title->rectTransform.pivot = { 0.5f, 1.0f };
    title->rectTransform.anchoredPosition = { 0.0f, -20.0f };
    title->rectTransform.sizeDelta = { 0.0f, 60.0f };
    title->SetText("UI Test Scene");
    title->SetFontSize(2.5f);
    title->SetColor(Engine::Color::white());
    title->SetAnchor(TextAnchor::MiddleCenter);

    // 3. Create a vertical layout group for the main menu buttons
    VerticalLayoutGroup* vlg = panel->CreateChild<VerticalLayoutGroup>();
    vlg->name = "MenuLayout";
    vlg->rectTransform.anchorMin = { 0.0f, 0.3f };
    vlg->rectTransform.anchorMax = { 1.0f, 0.8f };
    vlg->rectTransform.anchoredPosition = { 0.0f, 0.0f };
    vlg->rectTransform.sizeDelta = { -40.0f, 0.0f }; // Horizontal padding
    vlg->SetSpacing(15.0f);
    vlg->SetChildAlignment(ChildAlignment::UpperCenter);
    vlg->SetChildForceExpandWidth(true);

    // Add buttons to the vertical layout
    const char* buttonNames[] = { "Start Game", "Options", "Exit" };
    for (int i = 0; i < 3; ++i) {
        Button* btn = vlg->CreateChild<Button>();
        btn->name = buttonNames[i];
        btn->rectTransform.sizeDelta = { 0.0f, 60.0f };
        btn->SetText(buttonNames[i]);
        btn->SetBackgroundColor(Engine::Color(0.3f, 0.3f, 0.5f, 1.0f));
        
        btn->onClick = [i, buttonNames]() {
            std::cout << "DEBUG: " << buttonNames[i] << " clicked!" << std::endl;
        };
    }

    // 4. Add a horizontal layout group at the bottom for some "badges"
    HorizontalLayoutGroup* hlg = panel->CreateChild<HorizontalLayoutGroup>();
    hlg->name = "BadgeLayout";
    hlg->rectTransform.anchorMin = { 0.0f, 0.0f };
    hlg->rectTransform.anchorMax = { 1.0f, 0.2f };
    hlg->rectTransform.anchoredPosition = { 0.0f, 20.0f };
    hlg->rectTransform.sizeDelta = { -40.0f, 0.0f };
    hlg->SetSpacing(20.0f);
    hlg->SetChildAlignment(ChildAlignment::MiddleCenter);

    // Add some color badges
    Engine::Color badgeColors[] = { Engine::Color::red(), Engine::Color::green(), Engine::Color::blue() };
    for (int i = 0; i < 3; ++i) {
        Image* badge = hlg->CreateChild<Image>();
        badge->name = "Badge";
        badge->rectTransform.sizeDelta = { 40.0f, 40.0f };
        badge->SetColor(badgeColors[i]);
    }
    
    std::cout << "UITestScene Loaded: Canvas and Hierarchy initialized." << std::endl;
}

void UITestScene::OnUnload() {
    // UI System shutdown is now handled globally in Main.cpp
    std::cout << "UITestScene Unloaded." << std::endl;
}

void UITestScene::OnUpdate() {
    // UI Update is now handled globally in Main.cpp
    panel->SetAnchoredPosition({ panel->GetAnchoredPosition().x, panel->GetAnchoredPosition().y + 1});
}

} // namespace Game
