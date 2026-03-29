/*!
 * \file AlphaEngine2.5D\Source\Game\Scenes\MainMenuScene\MainMenuScene.cpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief [AUTO] Added Doxygen file header
 */

#include "MainMenuScene.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <cmath>
#include <cstdlib>

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

    void ApplyLayoutRecursive(UI::UIElement* e, const Json::Value& layout)
    {
        if (!e) return;
        ApplyLayoutNode(e, layout);
        for (auto* c : e->GetChildren()) ApplyLayoutRecursive(c, layout);
    }

    void ApplyLayoutToCanvas(UI::Canvas* canvas, const Json::Value& layout)
    {
        if (!canvas || !layout.isObject()) return;
        for (auto* root : canvas->GetRootElements()) ApplyLayoutRecursive(root, layout);
    }

    struct MainMenuSceneConfig : public Engine::IConfig
    {
        Json::Value layout;
        std::string mainMenuBgmKey = "main_menu";

        Engine::Color cameraBackgroundColor = Engine::Color(0.0f, 0.0f, 0.0f, 1.0f);
        float fogPrecision = 10000.0f;
        float fogDistance = 0.7f;
        float fogMinVisibility = 0.0f;
        float fogMaxVisibility = 0.5f;

        float moveSpeed = 0.3f;
        float baseRotation = 180.0f;
        float baseHeight = 0.6f;
        float cameraStartX = 50.0f;
        float cameraStartY = 1.5f;

        float wrapStart = 50.0f;
        float wrapEnd = 90.0f;

        float enemyShadowAnimSpeed = 0.5f;
        float enemyEyesAnimSpeed = 0.5f;
        float enemySpawnX = 54.0f;
        float enemySpawnY = 1.5f;
        float enemyHeight = 0.2f;
        float enemyScale = 0.75f;
        float enemyInitialAlpha = 0.0f;
        float enemySightAheadOffset = 2.0f;
        float enemyAlphaLerpSpeed = 5.0f;
        float enemyCenterThreshold = 0.6f;
        float enemyTargetMinAlpha = 0.3f;
        float enemyTargetMaxAlpha = 1.0f;
        float enemyDarkness = 0.1f;
        float enemyVisibleMinTime = 8.0f;
        float enemyVisibleMaxTime = 15.0f;

        float titleTypewriterCharsPerSecond = 12.0f;
        float titlePulseSpeed = 3.0f;

        float menuHoverScaleMultiplier = 1.15f;
        float menuFontLerpSpeed = 10.0f;
        float menuAnimationCompleteTime = 0.1f;
        float menuInteractionDelay = 0.2f;

        float cameraParallaxRotationStrength = 3.0f;

        float crosshairWidth = 300.0f;
        float crosshairHeight = 300.0f;
        Engine::Color crosshairColor = Engine::Color(1.0f, 1.0f, 0.0f, 0.8f);
        int crosshairRenderOrder = 1;

        Engine::Color menuNormalColor = Engine::Color(0.6f, 0.6f, 0.6f, 1.0f);
        Engine::Color menuSelectedColor = Engine::Color(1.0f, 1.0f, 1.0f, 1.0f);

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
            mainMenuBgmKey = j.get("mainMenuBgmKey", mainMenuBgmKey).asString();

            cameraBackgroundColor = ParseColor(j, "cameraBackgroundColor", cameraBackgroundColor);
            fogPrecision = j.get("fogPrecision", fogPrecision).asFloat();
            fogDistance = j.get("fogDistance", fogDistance).asFloat();
            fogMinVisibility = j.get("fogMinVisibility", fogMinVisibility).asFloat();
            fogMaxVisibility = j.get("fogMaxVisibility", fogMaxVisibility).asFloat();

            moveSpeed = j.get("moveSpeed", moveSpeed).asFloat();
            baseRotation = j.get("baseRotation", baseRotation).asFloat();
            baseHeight = j.get("baseHeight", baseHeight).asFloat();
            cameraStartX = j.get("cameraStartX", cameraStartX).asFloat();
            cameraStartY = j.get("cameraStartY", cameraStartY).asFloat();

            wrapStart = j.get("wrapStart", wrapStart).asFloat();
            wrapEnd = j.get("wrapEnd", wrapEnd).asFloat();

            enemyShadowAnimSpeed = j.get("enemyShadowAnimSpeed", enemyShadowAnimSpeed).asFloat();
            enemyEyesAnimSpeed = j.get("enemyEyesAnimSpeed", enemyEyesAnimSpeed).asFloat();
            enemySpawnX = j.get("enemySpawnX", enemySpawnX).asFloat();
            enemySpawnY = j.get("enemySpawnY", enemySpawnY).asFloat();
            enemyHeight = j.get("enemyHeight", enemyHeight).asFloat();
            enemyScale = j.get("enemyScale", enemyScale).asFloat();
            enemyInitialAlpha = j.get("enemyInitialAlpha", enemyInitialAlpha).asFloat();
            enemySightAheadOffset = j.get("enemySightAheadOffset", enemySightAheadOffset).asFloat();
            enemyAlphaLerpSpeed = j.get("enemyAlphaLerpSpeed", enemyAlphaLerpSpeed).asFloat();
            enemyCenterThreshold = j.get("enemyCenterThreshold", enemyCenterThreshold).asFloat();
            enemyTargetMinAlpha = j.get("enemyTargetMinAlpha", enemyTargetMinAlpha).asFloat();
            enemyTargetMaxAlpha = j.get("enemyTargetMaxAlpha", enemyTargetMaxAlpha).asFloat();
            enemyDarkness = j.get("enemyDarkness", enemyDarkness).asFloat();
            enemyVisibleMinTime = j.get("enemyVisibleMinTime", enemyVisibleMinTime).asFloat();
            enemyVisibleMaxTime = j.get("enemyVisibleMaxTime", enemyVisibleMaxTime).asFloat();

            titleTypewriterCharsPerSecond = j.get("titleTypewriterCharsPerSecond", titleTypewriterCharsPerSecond).asFloat();
            titlePulseSpeed = j.get("titlePulseSpeed", titlePulseSpeed).asFloat();

            menuHoverScaleMultiplier = j.get("menuHoverScaleMultiplier", menuHoverScaleMultiplier).asFloat();
            menuFontLerpSpeed = j.get("menuFontLerpSpeed", menuFontLerpSpeed).asFloat();
            menuAnimationCompleteTime = j.get("menuAnimationCompleteTime", menuAnimationCompleteTime).asFloat();
            menuInteractionDelay = j.get("menuInteractionDelay", menuInteractionDelay).asFloat();

            cameraParallaxRotationStrength = j.get("cameraParallaxRotationStrength", cameraParallaxRotationStrength).asFloat();

            crosshairWidth = j.get("crosshairWidth", crosshairWidth).asFloat();
            crosshairHeight = j.get("crosshairHeight", crosshairHeight).asFloat();
            crosshairColor = ParseColor(j, "crosshairColor", crosshairColor);
            crosshairRenderOrder = j.get("crosshairRenderOrder", crosshairRenderOrder).asInt();

            menuNormalColor = ParseColor(j, "menuNormalColor", menuNormalColor);
            menuSelectedColor = ParseColor(j, "menuSelectedColor", menuSelectedColor);
        }
    };

    MainMenuSceneConfig g_mainMenuSceneConfig;
    bool g_mainMenuSceneConfigLoaded = false;

    const MainMenuSceneConfig& GetMainMenuSceneConfig()
    {
        if (!g_mainMenuSceneConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/main_menu_scene.json", g_mainMenuSceneConfig);
            g_mainMenuSceneConfigLoaded = true;
        }
        return g_mainMenuSceneConfig;
    }
}

namespace Game {

    MainMenuScene::MainMenuScene() : Scene("MainMenuScene") {}

    /*!
     * \brief [AUTO] RandomFloat
     * \param min [in]
     * \param max [in]
     * \return 
     */
    /*!
     * \brief [AUTO] RandomFloat
     * \param min [in]
     * \param max [in]
     * \return 
     */
    /*!
     * \brief [AUTO] RandomFloat
     * \param min [in]
     * \param max [in]
     * \return 
     */
    /*!
     * \brief [AUTO] RandomFloat
     * \param min [in]
     * \param max [in]
     * \return 
     */
    float MainMenuScene::RandomFloat(float min, float max) {
        return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
    }

    /*!
     * \brief [AUTO] OnLoad
     */
    void MainMenuScene::OnLoad() {
        const auto& cfg = GetMainMenuSceneConfig();

        Input::RegisterCursorUnlock("MainMenuScene");
        SoundManager::PlayBGM(cfg.mainMenuBgmKey, true);

        transitionLocked = false;

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

        cam->backgroundColor = cfg.cameraBackgroundColor;
        cam->fogEnabled = true;
        cam->fogPrecision = cfg.fogPrecision;
        cam->fogDistance = cfg.fogDistance;
        cam->fogMinVisibility = cfg.fogMinVisibility;
        cam->fogMaxVisibility = cfg.fogMaxVisibility;

        moveSpeed = cfg.moveSpeed;
        baseRotation = cfg.baseRotation;
        baseHeight = cfg.baseHeight;
        enemyHeight = cfg.enemyHeight;

        cameraTf->position = Vector3(cfg.cameraStartX, cfg.cameraStartY, baseHeight);  // X=forward, Y=lateral (0.5=center), Z=height
        cameraTf->rotation = baseRotation;

        wrapStart = cfg.wrapStart;
        wrapEnd = cfg.wrapEnd;

        // ========== Enemy Shadow Setup ==========
		
        enemyShadow = CreateGameObject("EnemyShadow");
        auto shadowSprite = enemyShadow->AddComponent<SpriteSheetRenderer>();
        {
            SpriteSheet* shadowSheet = AssetManager::GetSpriteSheet("Assets/Sprites/guard.png");
            shadowSprite->SetSpriteSheet(shadowSheet);
            shadowSprite->SetIgnoreFog(true);
            shadowSprite->Play("Patrol");
            shadowSprite->SetSpeed(cfg.enemyShadowAnimSpeed);
        }
        shadowSprite->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));  // Start visible for debug
        shadowSprite->billboard = true;
        enemyShadow->transform->position = Vector3(cfg.enemySpawnX, cfg.enemySpawnY, enemyHeight);  // X=ahead, Y=center of corridor, Z=ground
        enemyShadow->transform->localScale = Vector2(cfg.enemyScale, cfg.enemyScale);

        // ========== Enemy Eyes Setup ==========
		
        enemyEyes = CreateGameObject("EnemyEyes");
        auto eyesSprite = enemyEyes->AddComponent<SpriteSheetRenderer>();
        {
            SpriteSheet* eyesSheet = AssetManager::GetSpriteSheet("Assets/Sprites/guard_eyes_overlay.png");
            eyesSprite->SetSpriteSheet(eyesSheet);
            eyesSprite->SetIgnoreFog(true);
            eyesSprite->Play("Patrol");
            eyesSprite->SetSpeed(cfg.enemyEyesAnimSpeed);
        }
        eyesSprite->billboard = true;
        enemyEyes->transform->position = Vector3(cfg.enemySpawnX, cfg.enemySpawnY, enemyHeight);  // X=ahead, Y=center of corridor, Z=ground
        enemyEyes->transform->localScale = Vector2(cfg.enemyScale, cfg.enemyScale);

        // Initialize enemy timing
        enemyTimer = 0.0f;
        nextEnemyTime = RandomFloat(cfg.enemyVisibleMinTime, cfg.enemyVisibleMaxTime);
        enemyVisible = false;
        enemyAlpha = cfg.enemyInitialAlpha;

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
            item.label->SetColor(cfg.menuNormalColor);  // Start visible
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
        crosshairImage->SetSizeDelta({ cfg.crosshairWidth, cfg.crosshairHeight });
		crosshairImage->SetColor(cfg.crosshairColor);
        crosshairImage->SetRenderOrder(cfg.crosshairRenderOrder); // Lowest Priority
        {
            Vector2 mousePos = Input::GetMousePosition();
            auto canvasPos = menuCanvas->ScreenToCanvasPosition(static_cast<s32>(mousePos.x), static_cast<s32>(mousePos.y));
            crosshairImage->SetAnchoredPosition({ canvasPos.x, canvasPos.y });
        }

        ApplyLayoutToCanvas(menuCanvas, cfg.layout);


         // Reset animation states
        typewriterTime = 0.0f;
        titleComplete = false;
        menuAnimTime = 0.0f;
        menuAnimComplete = false;
    }

    /*!
     * \brief [AUTO] UpdateMenuVisuals
     */
    void MainMenuScene::UpdateMenuVisuals() {
        const auto& cfg = GetMainMenuSceneConfig();
        for (int i = 0; i < static_cast<int>(menuItems.size()); ++i) {
            bool isSelected = (i == selectedIndex);
            bool isHovered = (i == hoveredIndex);

            if (isSelected || isHovered) {
                menuItems[i].label->SetText("> " + menuItemTexts[i]);
                menuItems[i].label->SetColor(cfg.menuSelectedColor);
                menuItems[i].targetFontSize = menuItemFontSizes[i] * cfg.menuHoverScaleMultiplier;
            }
            else {
                menuItems[i].label->SetText(menuItemTexts[i]);
                menuItems[i].label->SetColor(cfg.menuNormalColor);
                menuItems[i].targetFontSize = menuItemFontSizes[i];
            }
        }
    }

    /*!
     * \brief [AUTO] UpdateMenuAnimations
     * \param dt [in]
     */
    /*!
     * \brief [AUTO] UpdateMenuAnimations
     * \param dt [in]
     */
    /*!
     * \brief [AUTO] UpdateMenuAnimations
     * \param dt [in]
     */
    /*!
     * \brief [AUTO] UpdateMenuAnimations
     * \param dt [in]
     */
    void MainMenuScene::UpdateMenuAnimations(float dt) {
        const auto& cfg = GetMainMenuSceneConfig();
        menuAnimTime += dt;
        
        // Mark animation as complete immediately (no slide-in for now)
        if (!menuAnimComplete && menuAnimTime > cfg.menuAnimationCompleteTime) {
            menuAnimComplete = true;
        }

        // Smooth font size lerping
        for (int i = 0; i < static_cast<int>(menuItems.size()); ++i) {
            float lerpSpeed = cfg.menuFontLerpSpeed;
            menuItems[i].currentFontSize = Mathf::Lerp(
                menuItems[i].currentFontSize,
                menuItems[i].targetFontSize,
                dt * lerpSpeed
            );
            menuItems[i].label->SetFontSize(menuItems[i].currentFontSize);
        }
    }

    /*!
     * \brief [AUTO] UpdateEnemySighting
     * \param dt [in]
     */
    /*!
     * \brief [AUTO] UpdateEnemySighting
     * \param dt [in]
     */
    /*!
     * \brief [AUTO] UpdateEnemySighting
     * \param dt [in]
     */
    /*!
     * \brief [AUTO] UpdateEnemySighting
     * \param dt [in]
     */
    void MainMenuScene::UpdateEnemySighting(float dt) {
        const auto& cfg = GetMainMenuSceneConfig();
        if (!enemyShadow) return;

        auto shadowSprite = enemyShadow->GetComponent<SpriteSheetRenderer>();
        if (!shadowSprite) return;

        // Camera faces backwards (rotation ~180), so enemy should be at LOWER X to be in front
        float enemyX = cameraTf->position.x - cfg.enemySightAheadOffset;
        
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
        float threshold = cfg.enemyCenterThreshold;
        float targetAlpha = 0.0f;
        
        // If mouse is within proximity, and flashlight is on, increase silhouette visibility
        if (centerProximity > threshold && crosshairImage && crosshairImage->visible) {
            float t = (centerProximity - threshold) / (1.0f - threshold);
            targetAlpha = cfg.enemyTargetMinAlpha + t * (cfg.enemyTargetMaxAlpha - cfg.enemyTargetMinAlpha);
        }

        // Smooth alpha transition
        enemyAlpha = Mathf::Lerp(enemyAlpha, targetAlpha, dt * cfg.enemyAlphaLerpSpeed);

        // Drive guard loop from the lerped alpha for smoother audio transitions
        if (enemyAlpha > 0.01f) {
            const auto& soundCfg = SoundManager::GetGuardLoopConfig();
            const float clampedAlpha = Mathf::Clamp(enemyAlpha, 0.0f, 1.0f);
            const float lerpedDistance = Mathf::Lerp(soundCfg.farDistance, soundCfg.nearDistance, clampedAlpha);
            SoundManager::UpdateTerminalBuzzLoop(lerpedDistance);
        } else {
            SoundManager::UpdateTerminalBuzzLoop(-1.0f); // Force silence when not visible
        }

        // Apply dark silhouette effect (darkened sprite with alpha)
        float darkness = cfg.enemyDarkness;
        shadowSprite->SetColor(Color(darkness, darkness, darkness, enemyAlpha));

        // Makes sure enemyEyes follow the shadow position
        if (enemyEyes) {
            enemyEyes->transform->position = enemyShadow->transform->position;
        }
    }

    /*!
     * \brief [AUTO] SelectMenuItem
     * \param index [in]
     */
    /*!
     * \brief [AUTO] SelectMenuItem
     * \param index [in]
     */
    /*!
     * \brief [AUTO] SelectMenuItem
     * \param index [in]
     */
    /*!
     * \brief [AUTO] SelectMenuItem
     * \param index [in]
     */
    void MainMenuScene::SelectMenuItem(int index) {
        if (index >= 0 && index < static_cast<int>(menuItems.size())) {
            if (menuItems[index].onClick) {
                menuItems[index].onClick();
            }
        }
    }

    /*!
     * \brief [AUTO] OnUnload
     */
    void MainMenuScene::OnUnload() {
        Input::UnregisterCursorUnlock("MainMenuScene");

        transitionLocked = false;

        SoundManager::UpdateGuardProximityLoop(-1.0f);  // Force silence the guard proximity loop when leaving the main menu

        if (menuCanvas) {
            UIManager::GetInstance()->DestroyCanvas(menuCanvas);
            menuCanvas = nullptr;
        }
        titleText = nullptr;
        crosshairImage = nullptr;
        enemyShadow = nullptr;
        enemyEyes = nullptr;

        menuItems.clear();
        menuItemTexts.clear();
        menuItemFontSizes.clear();

       
    }

    /*!
     * \brief [AUTO] OnUpdate
     */
    void MainMenuScene::OnUpdate() {
        const auto& cfg = GetMainMenuSceneConfig();
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

        cameraTf->rotation = baseRotation - normalizedMouseX * cfg.cameraParallaxRotationStrength;

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
        const bool isControlsOpen = (controlsMenu && controlsMenu->IsVisible());
        const bool isTopUIBlockingMenu = (isDialogOpen || isControlsOpen || transitionLocked);

        // Only allow menu interaction after a short delay and when no blocking UI is open
        if (menuAnimTime > cfg.menuInteractionDelay && !isTopUIBlockingMenu) {
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
        }
        else if (isTopUIBlockingMenu && hoveredIndex != -1) {
            hoveredIndex = -1;
            UpdateMenuVisuals();
        }

        // Update menu animations (slide-in + font lerp)
        UpdateMenuAnimations(dt);

        // Typewriter effect for title
        if (titleText) {
            if (!titleComplete) {
                typewriterTime += dt;
                int charsToShow = static_cast<int>(typewriterTime * cfg.titleTypewriterCharsPerSecond);

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
                float pulse = 0.5f + 0.5f * Mathf::Sin(titlePulseTime * cfg.titlePulseSpeed);
                float alpha = 0.5f + 0.5f * pulse;
                titleText->SetColor(Color(1.0f, 1.0f, 1.0f, alpha));
            }
        }
    }

    /*!
     * \brief [AUTO] OnPlayClicked
     */
    void MainMenuScene::OnPlayClicked() {
        if (transitionLocked) return;
        transitionLocked = true;

        SoundManager::PlaySFX("ui_click");
        SceneManager::LoadWithLoadingScreen("FPS");
    }

    /*!
     * \brief [AUTO] OnControlsClicked
     */
    void MainMenuScene::OnControlsClicked() {
        SoundManager::PlaySFX("ui_click");
        if (controlsMenu)
        {
            controlsMenu->Show();
        }
    }

    /*!
     * \brief [AUTO] OnCreditsClicked
     */
    void MainMenuScene::OnCreditsClicked() {
        if (transitionLocked) return;
        transitionLocked = true;

        SoundManager::PlaySFX("ui_click");
        SceneManager::LoadWithFade("Credits");
    }

    /*!
     * \brief [AUTO] OnQuitClicked
     */
    void MainMenuScene::OnQuitClicked() {
        SoundManager::PlaySFX("ui_click");
        DialogService::ShowConfirmation(
            "Are you sure you want to quit?",
            "Quit", "Cancel",
            []() {
                Application::Quit();
            }
        );
    }

} // namespace Game
