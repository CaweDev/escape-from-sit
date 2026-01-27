/**
 * @file SceneRegistry.hpp
 * @brief Central registration of all game scenes
 */
#pragma once

#include "Engine/Scene/SceneManager.hpp"
#include "Game/Scenes/FPSScene/FPSScene.hpp"
#include "Game/Scenes/UITestScene/UITestScene.hpp"
#include "Game/Scenes/InteractionDemoScene/InteractionDemoScene.hpp"

namespace Game {

using namespace Engine;

// Scene instances
inline FPSScene fpsScene;
inline UITestScene uiTestScene;
inline InteractionDemoScene interactionDemoScene;

/**
 * @brief Register all game scenes with the SceneManager
 * Call this once after engine initialization
 */
inline void RegisterAllScenes() {
    SceneManager::RegisterScene("InteractionDemo", &interactionDemoScene);
    SceneManager::RegisterScene("FPS", &fpsScene);
    SceneManager::RegisterScene("UI", &uiTestScene);
}

} // namespace Game

