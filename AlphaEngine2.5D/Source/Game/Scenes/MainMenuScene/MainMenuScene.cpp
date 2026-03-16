#include "MainMenuScene.hpp"
#include <iostream>
#include <cmath>
#include <cstdlib>

namespace Game {

    MainMenuScene::MainMenuScene() : Scene("MainMenuScene") {}

    float MainMenuScene::RandomFloat(float min, float max) {
        return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
    }

    void MainMenuScene::OnLoad() {
        Input::RegisterCursorUnlock("MainMenuScene");
        SoundManager::PlayBGM("main_menu", true);

        // Seed random
        srand(static_cast<unsigned>(time(nullptr)));

        // Build corridor map data
        std::vector<int> corridorData(CORRIDOR_LENGTH * CORRIDOR_HEIGHT, 0);

        for (int x = 0; x < CORRIDOR_LENGTH; ++x) {
            corridorData[0 * CORRIDOR_LENGTH + x] = 1;
            corridorData[2 * CORRIDOR_LENGTH + x] = 1;
        }

        auto world = CreateGameObject("World");
        tileMap = world->AddComponent<TileMap>();
        tileMap->LoadFromArray(corridorData.data(), CORRIDOR_LENGTH, CORRIDOR_HEIGHT);

        auto renderer = world->AddComponent<TileMapRenderer>();
        renderer->SetCeilingSprite(AssetManager::GetSprite("Assets/Tilemap/Wallpaper_02_White_1.png"));
        renderer->SetFloorSprite(AssetManager::GetSprite("Assets/Tilemap/s2 Concrete Squares 2 Grey.png"));
        renderer->SetWallSprite(AssetManager::GetSprite("Assets/Tilemap/Concrete_01_White_1.png"));

        auto cameraGO = CreateGameObject("Camera");
        auto cam = cameraGO->AddComponent<Camera>();
        cameraTf = cameraGO->transform;

        cam->backgroundColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
        cam->fogEnabled = true;
        cam->fogPrecision = 10000.f;
        cam->fogDistance = 0.7f;
        cam->fogMinVisibility = 0.f;
        cam->fogMaxVisibility = 0.5f;

        cameraTf->position = Vector3(50.0f, 1.5f, baseHeight);  // X=forward, Y=lateral (0.5=center), Z=height
        cameraTf->rotation = baseRotation;

        wrapStart = 50.0f;
        wrapEnd = 90.0f;

        // ========== Enemy Shadow Setup ==========
		
        enemyShadow = CreateGameObject("EnemyShadow");
        auto shadowSprite = enemyShadow->AddComponent<SpriteSheetRenderer>();
        {
            SpriteSheet* shadowSheet = AssetManager::GetSpriteSheet("Assets/guard.png");
            shadowSprite->SetSpriteSheet(shadowSheet);
            shadowSprite->SetIgnoreFog(true);
            shadowSprite->Play("Patrol");
        }
        shadowSprite->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));  // Start visible for debug
        shadowSprite->billboard = true;
        enemyShadow->transform->position = Vector3(54.0f, 1.5f, enemyHeight);  // X=ahead, Y=center of corridor, Z=ground
        enemyShadow->transform->localScale = Vector2(0.75f, 0.75f);

        // Initialize enemy timing
        enemyTimer = 0.0f;
        nextEnemyTime = RandomFloat(8.0f, 15.0f);  // First appearance between 8-15 seconds
        enemyVisible = false;
        enemyAlpha = 0.0f;

        // ========== UI Setup ==========
        menuCanvas = UIManager::GetInstance()->CreateCanvas();
        menuCanvas->SetReferenceResolution(1280.f, 720.f);
        menuCanvas->SetScaleMode(CanvasScaleMode::ScaleWithScreenSize);
        menuCanvas->SetSortOrder(100);

        // Add ControlsMenu component
        auto controlsGO = CreateGameObject("ControlsMenu");
        controlsMenu = controlsGO->AddComponent<ControlsMenu>();

        // === Title (centered at top) - starts empty for typewriter effect ===
        titleText = menuCanvas->CreateChild<Text>();
        titleText->name = "MainTitle";
        titleText->SetAnchors({ 0.0f, 0.82f }, { 1.0f, 0.95f });
        titleText->SetAnchoredPosition({ 0.0f, 0.0f });
        titleText->SetSizeDelta({ 0.0f, 0.0f });
        titleText->SetText("");  // Start empty for typewriter
        titleText->SetFontSize(5.0f);
        titleText->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
        titleText->SetAnchor(TextAnchor::MiddleCenter);
        titleText->SetRenderOrder(100);

        // === Thin white line below title ===
        Image* titleUnderline = menuCanvas->CreateChild<Image>();
        titleUnderline->name = "TitleUnderline";
        titleUnderline->SetAnchors({ 0.3f, 0.80f }, { 0.7f, 0.80f });
        titleUnderline->SetPivot({ 0.5f, 0.5f });
        titleUnderline->SetAnchoredPosition({ 0.0f, 0.0f });
        titleUnderline->SetSizeDelta({ 0.0f, 2.0f });
        titleUnderline->SetColor(Color(1.0f, 1.0f, 1.0f, 0.7f));
		titleUnderline->SetRenderOrder(100);

        // === Menu container (right side, below title) ===
        Panel* menuPanel = menuCanvas->CreateChild<Panel>();
        menuPanel->name = "MenuPanel";
        menuPanel->SetAnchors({ 0.55f, 0.15f }, { 0.98f, 0.75f });
        menuPanel->SetAnchoredPosition({ 0.0f, 0.0f });
        menuPanel->SetSizeDelta({ 0.0f, 0.0f });
		menuPanel->SetRenderOrder(100);
        // No background - transparent

        // Menu items configuration
        struct MenuConfig {
            const char* text;
            float fontSize;
            std::function<void()> callback;
        };

        MenuConfig configs[] = {
            { "Play",     3.2f, [this]() { OnPlayClicked(); } },
            { "Controls", 1.8f, [this]() { OnControlsClicked(); } },
            { "Credits",  1.8f, [this]() { OnCreditsClicked(); } },
            { "Quit",     1.8f, [this]() { OnQuitClicked(); } }
        };

        // Create menu items
        float yPositions[] = { 0.85f, 0.65f, 0.45f, 0.25f };

        for (int i = 0; i < 4; ++i) {
            MenuItem item;

            item.label = menuPanel->CreateChild<Text>();
            item.label->name = configs[i].text;
            item.label->SetAnchors({ 0.4f, yPositions[i] - 0.06f }, { 1.0f, yPositions[i] + 0.06f });
            item.label->SetAnchoredPosition({ 0.0f, 0.0f });  // Start at final position
            item.label->SetSizeDelta({ 0.0f, 0.0f });
            item.label->SetText(configs[i].text);
            item.label->SetFontSize(configs[i].fontSize);
            item.label->SetColor(Color(0.6f, 0.6f, 0.6f, 1.0f));  // Start visible
            item.label->SetAnchor(TextAnchor::MiddleRight);

            item.selector = nullptr;
            item.onClick = configs[i].callback;

            // Initialize animation state
            item.currentFontSize = configs[i].fontSize;
            item.targetFontSize = configs[i].fontSize;
            item.slideOffset = 0.0f;  // No slide animation
            item.targetSlideOffset = 0.0f;

            menuItems.push_back(item);
            menuItemFontSizes.push_back(configs[i].fontSize);
        }

        // Store original text for each item
        menuItemTexts = { "Play", "Controls", "Credits", "Quit" };

        // Select first item (Play)
        selectedIndex = 0;
        UpdateMenuVisuals();  // Apply initial selection state

        // === Footer ===
        Text* versionText = menuCanvas->CreateChild<Text>();
        versionText->name = "VersionText";
        versionText->SetAnchors({ 0.02f, 0.02f }, { 0.3f, 0.08f });
        versionText->SetAnchoredPosition({ 0.0f, 0.0f });
        versionText->SetSizeDelta({ 0.0f, 0.0f });
        versionText->SetText("v0.1.0");
        versionText->SetFontSize(1.4f);
        versionText->SetColor(Color(0.5f, 0.5f, 0.5f, 0.8f));
        versionText->SetAnchor(TextAnchor::LowerLeft);

        Text* copyrightText = menuCanvas->CreateChild<Text>();
        copyrightText->name = "CopyrightText";
        copyrightText->SetAnchors({ 0.7f, 0.02f }, { 0.98f, 0.08f });
        copyrightText->SetAnchoredPosition({ 0.0f, 0.0f });
        copyrightText->SetSizeDelta({ 0.0f, 0.0f });
        copyrightText->SetText("DigiPen Institute of Technology");
        copyrightText->SetFontSize(1.4f);
        copyrightText->SetColor(Color(0.5f, 0.5f, 0.5f, 0.8f));
        copyrightText->SetAnchor(TextAnchor::LowerRight);

        // === Crosshair (follows mouse) ===
        crosshairImage = menuCanvas->CreateChild<SpriteSheetImage>();
        crosshairImage->name = "Flashlight";
        {
            SpriteSheet* crosshairSheet = AssetManager::GetSpriteSheet("Assets/UI/flashlight_spritesheet.png", 1, 20);
			crosshairSheet->AddAnimation("Flash", 0, 20, 30.f, true);  // Flash animation
            
            //SpriteSheet* crosshairSheet = AssetManager::GetSpriteSheet("Assets/UI/flashlight.png", 1, 2);
			//crosshairSheet->AddAnimation("Flash", 0, 2, 10.f, true);  // Flash animation
            
            crosshairImage->SetSpriteSheet(crosshairSheet);
			crosshairImage->Play("Flash");
        }
        crosshairImage->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
        crosshairImage->SetPivot({ 0.5f, 0.5f });
        crosshairImage->SetSizeDelta({ 300.f, 300.f });
		crosshairImage->SetColor(Color(1.0f, 1.0f, 0.f, 0.8f));  // Yellow
        crosshairImage->SetRenderOrder(1); // Lowest Priority
        {
            Vector2 mousePos = Input::GetMousePosition();
            auto canvasPos = menuCanvas->ScreenToCanvasPosition(static_cast<s32>(mousePos.x), static_cast<s32>(mousePos.y));
            crosshairImage->SetAnchoredPosition({ canvasPos.x, canvasPos.y });
        }
      

        // Reset animation states
        typewriterTime = 0.0f;
        titleComplete = false;
        menuAnimTime = 0.0f;
        menuAnimComplete = false;
    }

    void MainMenuScene::UpdateMenuVisuals() {
        for (int i = 0; i < static_cast<int>(menuItems.size()); ++i) {
            bool isSelected = (i == selectedIndex);
            bool isHovered = (i == hoveredIndex);

            if (isSelected || isHovered) {
                menuItems[i].label->SetText("> " + menuItemTexts[i]);
                menuItems[i].label->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
                menuItems[i].targetFontSize = menuItemFontSizes[i] * 1.15f;
            }
            else {
                menuItems[i].label->SetText(menuItemTexts[i]);
                menuItems[i].label->SetColor(Color(0.6f, 0.6f, 0.6f, 1.0f));
                menuItems[i].targetFontSize = menuItemFontSizes[i];
            }
        }
    }

    void MainMenuScene::UpdateMenuAnimations(float dt) {
        menuAnimTime += dt;
        
        // Mark animation as complete immediately (no slide-in for now)
        if (!menuAnimComplete && menuAnimTime > 0.1f) {
            menuAnimComplete = true;
        }

        // Smooth font size lerping
        for (int i = 0; i < static_cast<int>(menuItems.size()); ++i) {
            float lerpSpeed = 10.0f;
            menuItems[i].currentFontSize = Mathf::Lerp(
                menuItems[i].currentFontSize,
                menuItems[i].targetFontSize,
                dt * lerpSpeed
            );
            menuItems[i].label->SetFontSize(menuItems[i].currentFontSize);
        }
    }

    void MainMenuScene::UpdateEnemySighting(float dt) {
        if (!enemyShadow) return;

        auto shadowSprite = enemyShadow->GetComponent<SpriteSheetRenderer>();
        if (!shadowSprite) return;

        // Camera faces backwards (rotation ~180), so enemy should be at LOWER X to be in front
        float enemyX = cameraTf->position.x - 2.0f;
        
        // Clamp to valid corridor range
        if (enemyX < 0.0f) enemyX = 0.0f;
        if (enemyX > static_cast<float>(CORRIDOR_LENGTH)) enemyX = static_cast<float>(CORRIDOR_LENGTH);
        
        enemyShadow->transform->position = Vector3(enemyX, 1.5f, enemyHeight);

        // Get mouse position relative to screen center
        s32 mouseX, mouseY;
        AEInputGetCursorPosition(&mouseX, &mouseY);

        float screenWidth = static_cast<float>(Application::GetWindowWidth());
        float screenHeight = static_cast<float>(Application::GetWindowHeight());

        // Calculate distance from center (0 = center, 1 = edge)
        float centerX = screenWidth * 0.5f;
        float centerY = screenHeight * 0.5f;
        
        float distX = std::abs(mouseX - centerX) / centerX;
        float distY = std::abs(mouseY - centerY) / centerY;
        float distFromCenter = std::sqrt(distX * distX + distY * distY);
        
        // Normalize to 0-1 range (closer to center = higher value)
        float centerProximity = 1.0f - Mathf::Clamp(distFromCenter, 0.0f, 1.0f);
        
        // Only show when mouse is near center (within ~40% of screen)
        float threshold = 0.6f;
        float targetAlpha = 0.3f;
        
		// If mouse is within proximity, and flashlight is on, increase silhouette visibility
        if (centerProximity > threshold && crosshairImage->visible) {
            float t = (centerProximity - threshold) / (1.0f - threshold);
            targetAlpha += t * 0.7f;  // Max 100% opacity
        }

        // Smooth alpha transition
        enemyAlpha = Mathf::Lerp(enemyAlpha, targetAlpha, dt * 5.0f);

        // Apply dark silhouette effect (darkened sprite with alpha)
        float darkness = 0.1f;  // Very dark silhouette
        shadowSprite->SetColor(Color(darkness, darkness, darkness, enemyAlpha));
    }

    void MainMenuScene::SelectMenuItem(int index) {
        if (index >= 0 && index < static_cast<int>(menuItems.size())) {
            if (menuItems[index].onClick) {
                menuItems[index].onClick();
            }
        }
    }

    void MainMenuScene::OnUnload() {
        Input::UnregisterCursorUnlock("MainMenuScene");
        if (menuCanvas) {
            UIManager::GetInstance()->DestroyCanvas(menuCanvas);
            menuCanvas = nullptr;
        }
        titleText = nullptr;
        crosshairImage = nullptr;
        enemyShadow = nullptr;

        menuItems.clear();
        menuItemTexts.clear();
        menuItemFontSizes.clear();
    }

    void MainMenuScene::OnUpdate() {
        if (!cameraTf) return;

        float dt = Time::unscaledDeltaTime;

        // Camera forward movement
        cameraTf->position.x += moveSpeed * dt;

        if (cameraTf->position.x >= wrapEnd) {
            cameraTf->position.x = wrapStart + (cameraTf->position.x - wrapEnd);
        }

        // Parallax: subtle camera sway based on mouse position
        s32 mouseX, mouseY;
        AEInputGetCursorPosition(&mouseX, &mouseY);

        float screenWidth = static_cast<float>(Application::GetWindowWidth());

        float normalizedMouseX = (mouseX / screenWidth - 0.5f) * 2.0f;

        cameraTf->rotation = baseRotation - normalizedMouseX * 3.0f;

        // Update crosshair position to follow mouse
        if (crosshairImage) {
            auto canvasPos = menuCanvas->ScreenToCanvasPosition(mouseX, mouseY);
            crosshairImage->SetAnchoredPosition({ canvasPos.x, canvasPos.y });

            // If right click pressed
            if (Input::GetMouseButtonDown(1)) {
                crosshairImage->visible = !crosshairImage->visible;  // Toggle visibility on right-click
            }
        }

        // Update enemy sighting
        UpdateEnemySighting(dt);

        const bool isDialogOpen = UI::DialogService::IsConfirmationVisible();

        // Only allow menu interaction after a short delay and when no modal dialog is open
        if (menuAnimTime > 0.2f && !isDialogOpen) {
            // Keyboard navigation
            if (Input::GetKeyDown(KeyCode::UpArrow) || Input::GetKeyDown(KeyCode::W)) {
                selectedIndex = (selectedIndex - 1 + static_cast<int>(menuItems.size())) % static_cast<int>(menuItems.size());
                SoundManager::PlaySFX("ui_hover", 0.7f);
                UpdateMenuVisuals();
            }
            if (Input::GetKeyDown(KeyCode::DownArrow) || Input::GetKeyDown(KeyCode::S)) {
                selectedIndex = (selectedIndex + 1) % static_cast<int>(menuItems.size());
                SoundManager::PlaySFX("ui_hover", 0.7f);
                UpdateMenuVisuals();
            }
            if (Input::GetKeyDown(KeyCode::Return) || Input::GetKeyDown(KeyCode::Space)) {
                SelectMenuItem(selectedIndex);
            }

            // Mouse hover detection
            auto canvasPos = menuCanvas->ScreenToCanvasPosition(mouseX, mouseY);
            f32 worldX = canvasPos.x, worldY = canvasPos.y;

            int newHovered = -1;
            for (int i = 0; i < static_cast<int>(menuItems.size()); ++i) {
                Rect labelRect = menuItems[i].label->GetWorldRect();
                if (worldX >= labelRect.x && worldX <= labelRect.x + labelRect.width &&
                    worldY >= labelRect.y && worldY <= labelRect.y + labelRect.height) {
                    newHovered = i;
                    break;
                }
            }

            if (newHovered != hoveredIndex) {
                hoveredIndex = newHovered;
                if (hoveredIndex >= 0) {
                    selectedIndex = hoveredIndex;
                    SoundManager::PlaySFX("ui_hover", 0.7f);
                }
                UpdateMenuVisuals();
            }

            // Mouse click
            if (Input::GetMouseButtonDown(0) && hoveredIndex >= 0) {
                SelectMenuItem(hoveredIndex);
            }
        } else if (isDialogOpen && hoveredIndex != -1) {
            hoveredIndex = -1;
            UpdateMenuVisuals();
        }

        // Update menu animations (slide-in + font lerp)
        UpdateMenuAnimations(dt);

        // Typewriter effect for title
        if (titleText) {
            if (!titleComplete) {
                typewriterTime += dt;
                int charsToShow = static_cast<int>(typewriterTime * 12.0f);

                if (charsToShow >= static_cast<int>(fullTitle.length())) {
                    titleText->SetText(fullTitle);
                    titleComplete = true;
                    titlePulseTime = 0.0f;
                }
                else {
                    titleText->SetText(fullTitle.substr(0, charsToShow));
                }
            }
            else {
                titlePulseTime += dt;
                float pulse = 0.5f + 0.5f * Mathf::Sin(titlePulseTime * 3.f);
                float alpha = 0.5f + 0.5f * pulse;
                titleText->SetColor(Color(1.0f, 1.0f, 1.0f, alpha));
            }
        }
    }

    void MainMenuScene::OnPlayClicked() {
        SoundManager::PlaySFX("ui_click");
        std::cout << "Play clicked - Loading game scene..." << std::endl;
        SceneManager::LoadWithLoadingScreen("FPS");
    }

    void MainMenuScene::OnControlsClicked() {
        SoundManager::PlaySFX("ui_click");
        if (controlsMenu)
        {
            controlsMenu->Show();
        }
    }

    void MainMenuScene::OnCreditsClicked() {
        SoundManager::PlaySFX("ui_click");
        std::cout << "Credits clicked - Loading credit scene..." << std::endl;
        SceneManager::LoadWithFade("Credits");
    }

    void MainMenuScene::OnQuitClicked() {
        SoundManager::PlaySFX("ui_click");
        DialogService::ShowConfirmation(
            "Are you sure you want to quit?",
            "Quit", "Cancel",
            []() {
                std::cout << "Quit confirmed - Exiting game..." << std::endl;
                Application::Quit();
            }
        );
    }

} // namespace Game