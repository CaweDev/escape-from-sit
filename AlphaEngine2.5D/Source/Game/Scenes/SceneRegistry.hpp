/**
 * @file SceneRegistry.hpp
 * @brief Central registration of all game scenes
 */
#pragma once

#include "Engine/Scene/SceneManager.hpp"
#include "Game/Scenes/FPSScene/FPSScene.hpp"

namespace Game {

using namespace Engine;

// Scene instances
inline FPSScene fpsScene;

/**
 * @brief Register all game scenes with the SceneManager
 * Call this once after engine initialization
 */
inline void RegisterAllScenes() {
    SceneManager::RegisterScene("FPS", &fpsScene);
}

} // namespace Game
