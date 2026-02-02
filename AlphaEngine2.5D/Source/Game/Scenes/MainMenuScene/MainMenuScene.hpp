#pragma once

#include "Engine.hpp"

namespace Game {

using namespace Engine;

/**
 * @class MainMenuScene
 * @brief Scene for main menu background with infinite corridor effect
 * 
 * Renders a 1-tile wide corridor with the camera automatically moving
 * backward while pointing straight, creating a smooth infinite scroll.
 */
class MainMenuScene : public Scene {
public:
    MainMenuScene();

    // Camera movement settings
    float moveSpeed = 0.3f;  // Units per second (backward)

    void OnLoad() override;
    void OnUpdate() override;
    
private:
    Transform* cameraTf = nullptr;
    TileMap* tileMap = nullptr;
    
    // Corridor dimensions
    static constexpr int CORRIDOR_LENGTH = 100;
    static constexpr int CORRIDOR_HEIGHT = 3;
    
    // Wrap position for infinite loop
    float wrapStart = 5.0f;
    float wrapEnd = 0.0f;
};

} // namespace Game
