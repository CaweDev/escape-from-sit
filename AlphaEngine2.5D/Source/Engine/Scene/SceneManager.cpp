/**
 * @file SceneManager.cpp
 * @brief Implementation of SceneManager class
 */
#include "SceneManager.hpp"
#include "Engine/UI/Core/UIManager.hpp"

namespace Engine {

f32 SceneManager::s_defaultLoadScreenFadeInDuration = 1.f;
f32 SceneManager::s_defaultLoadScreenFadeOutDuration = 1.f;
Scene* SceneManager::s_activeScene = nullptr;
std::unordered_map<std::string, Scene*> SceneManager::s_sceneRegistry;
std::string SceneManager::s_pendingScene;
bool SceneManager::s_hasPendingScene = false;
std::string SceneManager::s_loadWithScreenTarget;
f32 SceneManager::s_loadWithScreenFadeInDuration = 1.f;
f32 SceneManager::s_loadWithScreenFadeOutDuration = 1.f;

void SceneManager::LoadScene(const std::string& sceneName) {
    auto it = s_sceneRegistry.find(sceneName);
    if (it != s_sceneRegistry.end()) {
        LoadSceneInternal(it->second);
    }
}

void SceneManager::LoadScene(int buildIndex) {
    for (auto& pair : s_sceneRegistry) {
        if (pair.second->buildIndex == buildIndex) {
            LoadSceneInternal(pair.second);
            return;
        }
    }
}

Scene* SceneManager::GetActiveScene() {
    return s_activeScene;
}

void SceneManager::RegisterScene(const std::string& name, Scene* scene) {
    scene->name = name;
    scene->buildIndex = static_cast<int>(s_sceneRegistry.size());
    s_sceneRegistry[name] = scene;
}

int SceneManager::GetSceneCount() {
    return static_cast<int>(s_sceneRegistry.size());
}

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

void SceneManager::LoadSceneDeferred(const std::string& sceneName) {
    s_pendingScene = sceneName;
    s_hasPendingScene = true;
}

void SceneManager::LoadWithLoadingScreen(const std::string& sceneName) {
    LoadWithLoadingScreen(sceneName, s_defaultLoadScreenFadeInDuration, s_defaultLoadScreenFadeOutDuration);
}

void SceneManager::LoadWithLoadingScreen(const std::string& sceneName, f32 fadeInDuration, f32 fadeOutDuration) {
    s_loadWithScreenTarget = sceneName;
    s_loadWithScreenFadeInDuration = fadeInDuration;
    s_loadWithScreenFadeOutDuration = fadeOutDuration;

    auto* ui = UI::UIManager::GetInstance();
    if (ui) {
        ui->FadeIn(fadeInDuration, []() {
            LoadScene("Loading");
        });
    }
    else {
        // Fallback: load directly if UI not available
        LoadScene(sceneName);
    }
}

void SceneManager::LoadWithFade(const std::string& sceneName) {
    LoadWithFade(sceneName, s_defaultLoadScreenFadeInDuration, s_defaultLoadScreenFadeOutDuration);
}

void SceneManager::LoadWithFade(const std::string& sceneName, f32 fadeInDuration, f32 fadeOutDuration) {
    auto* ui = UI::UIManager::GetInstance();
    if (ui) {
        ui->FadeIn(fadeInDuration, [sceneName, fadeOutDuration]() {
            LoadScene(sceneName);
            auto* ui = UI::UIManager::GetInstance();
            if (ui) {
                ui->FadeOut(fadeOutDuration);
            }
        });
    }
    else {
        LoadScene(sceneName);
    }
}

void SceneManager::RenderActiveScene() {
    if (s_activeScene && s_activeScene->IsLoaded()) {
        s_activeScene->RenderInternal();
    }
}

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

void SceneManager::UnloadActiveScene() {
    if (s_activeScene) {
        s_activeScene->UnloadInternal();
        s_activeScene = nullptr;
    }
}

void SceneManager::LoadSceneInternal(Scene* newScene) {
    if (!newScene) return;
    
    if (s_activeScene) {
        s_activeScene->UnloadInternal();
    }
    
    s_activeScene = newScene;
    s_activeScene->LoadInternal();
}

} // namespace Engine
