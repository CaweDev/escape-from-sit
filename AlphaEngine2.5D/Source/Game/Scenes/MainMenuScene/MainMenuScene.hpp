#pragma once

#include "Engine.hpp"
#include <vector>
#include <string>

namespace Game {

    using namespace Engine;
    using namespace UI;

    /**
     * @class MainMenuScene
     * @brief Scene for main menu background with infinite corridor effect
     */
    class MainMenuScene : public Scene {
    public:
        MainMenuScene();

        float moveSpeed = 0.3f;

        void OnLoad() override;
        void OnUnload() override;
        void OnUpdate() override;

    private:
        Transform* cameraTf = nullptr;
        TileMap* tileMap = nullptr;

        // UI
        Canvas* menuCanvas = nullptr;
        Text* titleText = nullptr;
        SpriteSheetImage* crosshairImage = nullptr;
        float titlePulseTime = 0.0f;

        // Menu items
        struct MenuItem {
            Text* label = nullptr;
            Text* selector = nullptr;
            std::function<void()> onClick;

            // Animation state
            float currentFontSize = 0.0f;
            float targetFontSize = 0.0f;
            float slideOffset = 0.0f;      // Current X offset for slide-in
            float targetSlideOffset = 0.0f; // Target X offset (0 when fully visible)
        };
        std::vector<MenuItem> menuItems;
        std::vector<std::string> menuItemTexts;
        std::vector<float> menuItemFontSizes;
        int selectedIndex = 0;
        int hoveredIndex = -1;

        // Slide-in animation
        float menuAnimTime = 0.0f;
        bool menuAnimComplete = false;

        // Typewriter effect
        std::string fullTitle = "ESCAPE FROM SIT";
        float typewriterTime = 0.0f;
        bool titleComplete = false;

        // Parallax
        float baseRotation = 180.0f;
        float baseHeight = 0.6f;

        // Enemy sighting
        GameObject* enemyShadow = nullptr;
        float enemyTimer = 0.0f;
        float nextEnemyTime = 0.0f;
        bool enemyVisible = false;
        float enemyVisibleDuration = 0.0f;
        float enemyAlpha = 0.0f;

        static constexpr int CORRIDOR_LENGTH = 100;
        static constexpr int CORRIDOR_HEIGHT = 3;

        float wrapStart = 5.0f;
        float wrapEnd = 0.0f;

        void UpdateMenuVisuals();
        void UpdateMenuAnimations(float dt);
        void UpdateEnemySighting(float dt);
        void SelectMenuItem(int index);
        float RandomFloat(float min, float max);

        void OnPlayClicked();
        void OnControlsClicked();
        void OnCreditsClicked();
        void OnQuitClicked();
    };

} // namespace Game