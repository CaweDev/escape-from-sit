/**
 * @file SceneRegistry.hpp
 * @brief Central registration of all game scenes
 */
#pragma once
#include "Engine/Scene/SceneManager.hpp"
#include "Game/Scenes/FPSScene/FPSScene.hpp"
#include "Game/Scenes/UITestScene/UITestScene.hpp"
#include "Game/Scenes/InteractionDemoScene/InteractionDemoScene.hpp"
#include "Game/Scenes/MainMenuScene/MainMenuScene.hpp"
#include "Game/Scenes/LoadingScene/LoadingScene.hpp"
#include "Game/Scenes/SplashScene/SplashScene.hpp"
#include "Game/Scenes/CreditScene/CreditScene.hpp"
#include "Editor/EditorScene.hpp"

namespace Game
{

    using namespace Engine;

    // Scene instances
    inline EditorScene editorScene;
    inline FPSScene fpsScene;
    inline UITestScene uiTestScene;
    inline InteractionDemoScene interactionDemoScene;
    inline MainMenuScene mainMenuScene;
    inline LoadingScene loadingScene;
    inline SplashScene splashScene;
    inline CreditScene creditScene;

    /**
     * @brief Register all game scenes with the SceneManager
     * Call this once after engine initialization
     */
    inline void RegisterAllScenes()
    {
        SceneManager::RegisterScene("SplashScene", &splashScene);
        SceneManager::RegisterScene("Credits", &creditScene);
        SceneManager::RegisterScene("MainMenu", &mainMenuScene);
        SceneManager::RegisterScene("FPS", &fpsScene);
        SceneManager::RegisterScene("InteractionDemo", &interactionDemoScene);
        SceneManager::RegisterScene("UI", &uiTestScene);
        SceneManager::RegisterScene("Loading", &loadingScene);
        SceneManager::RegisterScene("EditorScene", &editorScene);
    }

} // namespace Game
