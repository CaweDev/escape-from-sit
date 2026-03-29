/**
 * @file SceneManager.cpp
 * @brief Implementation of SceneManager class
 */
#include "SceneManager.hpp"
#include "Engine/UI/Core/UIManager.hpp"
#include "Engine/UI/Helpers/FadeService.hpp"
#include "Engine/Audio/SoundManager.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <cstdio>

namespace Engine {

// Default values are read from file in initialize()
f32 SceneManager::s_defaultLoadScreenFadeInDuration  = 1.f;
f32 SceneManager::s_defaultLoadScreenFadeOutDuration = 1.f;
Scene* SceneManager::s_activeScene = nullptr;
std::unordered_map<std::string, Scene*> SceneManager::s_sceneRegistry;
std::string SceneManager::s_pendingScene;
bool SceneManager::s_hasPendingScene = false;
std::string SceneManager::s_loadWithScreenTarget;
f32 SceneManager::s_loadWithScreenFadeInDuration  = 1.f;
f32 SceneManager::s_loadWithScreenFadeOutDuration = 1.f;
bool SceneManager::s_hasPendingPostLoadFadeOut = false;
f32 SceneManager::s_pendingPostLoadFadeOutDuration = 0.0f;

void SceneManager::Initialize(const std::string& configPath)
{
    struct GameConfig : public IConfig {
        float fadeIn  = 1.0f;
        float fadeOut = 1.0f;
        void FromJson(const Json::Value& j) override {
            fadeIn  = j.get("sceneLoadFadeInDuration",  fadeIn).asFloat();
            fadeOut = j.get("sceneLoadFadeOutDuration", fadeOut).asFloat();
        }
    };
    GameConfig config;
    if (!ConfigManager::ReadConfig(configPath, config))
        std::printf("[Warning] SceneManager: Failed to read config '%s'. Using defaults.\n", configPath.c_str());
    s_defaultLoadScreenFadeInDuration  = config.fadeIn;
    s_defaultLoadScreenFadeOutDuration = config.fadeOut;
    s_loadWithScreenFadeInDuration     = config.fadeIn;
    s_loadWithScreenFadeOutDuration    = config.fadeOut;
}

/*!
 * \brief [AUTO] LoadScene
 * \param sceneName [in]
 */
void SceneManager::LoadScene(const std::string& sceneName) {
    auto it = s_sceneRegistry.find(sceneName);
    if (it != s_sceneRegistry.end()) {
        LoadSceneInternal(it->second);
    }
}

/*!
 * \brief [AUTO] LoadScene
 * \param buildIndex [in]
 */
void SceneManager::LoadScene(int buildIndex) {
    for (auto& pair : s_sceneRegistry) {
        if (pair.second->buildIndex == buildIndex) {
            LoadSceneInternal(pair.second);
            return;
        }
    }
}

/*!
 * \brief [AUTO] GetActiveScene
 * \return 
 */
Scene* SceneManager::GetActiveScene() {
    return s_activeScene;
}

/*!
 * \brief [AUTO] RegisterScene
 * \param name [in]
 * \param scene [in]
 */
void SceneManager::RegisterScene(const std::string& name, Scene* scene) {
    scene->name = name;
    scene->buildIndex = static_cast<int>(s_sceneRegistry.size());
    s_sceneRegistry[name] = scene;
}

/*!
 * \brief [AUTO] GetSceneCount
 * \return 
 */
int SceneManager::GetSceneCount() {
    return static_cast<int>(s_sceneRegistry.size());
}

/*!
 * \brief [AUTO] UpdateActiveScene
 */
void SceneManager::UpdateActiveScene() {
    if (s_activeScene && s_activeScene->IsLoaded()) {
        s_activeScene->UpdateInternal();
    }

    // Process deferred scene load at end of update cycle
    if (s_hasPendingScene) {
        s_hasPendingScene = false;
        std::string target = s_pendingScene;
        s_pendingScene.clear();
        LoadScene(target);
    }
}

/*!
 * \brief [AUTO] LoadSceneDeferred
 * \param sceneName [in]
 */
void SceneManager::LoadSceneDeferred(const std::string& sceneName) {
    s_pendingScene = sceneName;
    s_hasPendingScene = true;
}

/*!
 * \brief [AUTO] LoadWithLoadingScreen
 * \param sceneName [in]
 */
void SceneManager::LoadWithLoadingScreen(const std::string& sceneName) {
    LoadWithLoadingScreen(sceneName, s_defaultLoadScreenFadeInDuration, s_defaultLoadScreenFadeOutDuration);
}

/*!
 * \brief [AUTO] LoadWithLoadingScreen
 * \param sceneName [in]
 * \param fadeInDuration [in]
 * \param fadeOutDuration [in]
 */
void SceneManager::LoadWithLoadingScreen(const std::string& sceneName, f32 fadeInDuration, f32 fadeOutDuration) {
    s_loadWithScreenTarget = sceneName;
    s_loadWithScreenFadeInDuration = fadeInDuration;
    s_loadWithScreenFadeOutDuration = fadeOutDuration;

    SoundManager::StopBGM(); // stops bgm everytime loading screen appears

  /*!
   * \brief [AUTO] FadeIn
   * \param fadeInDuration [in]
   * \param []( [in]
   */
    UI::FadeService::FadeIn(fadeInDuration, []() {
        LoadScene("Loading");
    });
}

/*!
 * \brief [AUTO] LoadWithFade
 * \param sceneName [in]
 */
void SceneManager::LoadWithFade(const std::string& sceneName) {
    LoadWithFade(sceneName, s_defaultLoadScreenFadeInDuration, s_defaultLoadScreenFadeOutDuration);
}

/*!
 * \brief [AUTO] LoadWithFade
 * \param sceneName [in]
 * \param fadeInDuration [in]
 * \param fadeOutDuration [in]
 */
void SceneManager::LoadWithFade(const std::string& sceneName, f32 fadeInDuration, f32 fadeOutDuration) {
  /*!
   * \brief [AUTO] FadeIn
   * \param fadeInDuration [in]
   * \param [sceneName [in]
   * \param fadeOutDuration]( [in]
   */
    UI::FadeService::FadeIn(fadeInDuration, [sceneName, fadeOutDuration]() {
        LoadScene(sceneName);
        UI::FadeService::FadeOut(fadeOutDuration);
    });
}

/*!
 * \brief [AUTO] RenderActiveScene
 */
void SceneManager::RenderActiveScene() {
    if (s_activeScene && s_activeScene->IsLoaded()) {
        s_activeScene->RenderInternal();
    }
}

/*!
 * \brief [AUTO] LoadFirstScene
 */
void SceneManager::LoadFirstScene() {
    if (!s_sceneRegistry.empty()) {
        Scene* first = nullptr;
        for (auto& pair : s_sceneRegistry) {
            if (!first || pair.second->buildIndex < first->buildIndex) {
                first = pair.second;
            }
        }
        if (first) {
            LoadSceneInternal(first);
        }
    }
}

/*!
 * \brief [AUTO] UnloadActiveScene
 */
void SceneManager::UnloadActiveScene() {
    if (s_activeScene) {
        SoundManager::StopAllSFX();
        s_activeScene->UnloadInternal();
        s_activeScene = nullptr;
    }
}

/*!
 * \brief [AUTO] RequestFadeOutAfterNextSceneLoad
 * \param duration [in]
 */
void SceneManager::RequestFadeOutAfterNextSceneLoad(f32 duration) {
    s_hasPendingPostLoadFadeOut = true;
    s_pendingPostLoadFadeOutDuration = duration;
}

/*!
 * \brief [AUTO] LoadSceneInternal
 * \param newScene [in]
 */
void SceneManager::LoadSceneInternal(Scene* newScene) {
    if (!newScene) return;
    
    if (s_activeScene) {
        SoundManager::StopAllSFX();
        s_activeScene->UnloadInternal();
    }
    
    s_activeScene = newScene;
    s_activeScene->LoadInternal();

    if (s_hasPendingPostLoadFadeOut) {
        s_hasPendingPostLoadFadeOut = false;
        UI::FadeService::FadeOut(s_pendingPostLoadFadeOutDuration);
    }
}

} // namespace Engine
