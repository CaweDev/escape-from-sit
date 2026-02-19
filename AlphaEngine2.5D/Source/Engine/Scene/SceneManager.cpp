/**
 * @file SceneManager.cpp
 * @brief Implementation of SceneManager class
 */
#include "SceneManager.hpp"
#include "Engine/UI/Core/UIManager.hpp"

namespace Engine {

Scene* SceneManager::activeScene = nullptr;
std::unordered_map<std::string, Scene*> SceneManager::sceneRegistry;
std::string SceneManager::s_pendingScene;
bool SceneManager::s_hasPendingScene = false;
std::string SceneManager::s_loadWithScreenTarget;

void SceneManager::LoadScene(const std::string& sceneName) {
    auto it = sceneRegistry.find(sceneName);
    if (it != sceneRegistry.end()) {
        LoadSceneInternal(it->second);
    }
}

void SceneManager::LoadScene(int buildIndex) {
    for (auto& pair : sceneRegistry) {
        if (pair.second->buildIndex == buildIndex) {
            LoadSceneInternal(pair.second);
            return;
        }
    }
}

Scene* SceneManager::GetActiveScene() {
    return activeScene;
}

void SceneManager::RegisterScene(const std::string& name, Scene* scene) {
    scene->name = name;
    scene->buildIndex = static_cast<int>(sceneRegistry.size());
    sceneRegistry[name] = scene;
}

int SceneManager::GetSceneCount() {
    return static_cast<int>(sceneRegistry.size());
}

void SceneManager::UpdateActiveScene() {
    if (activeScene && activeScene->IsLoaded()) {
        activeScene->UpdateInternal();
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

void SceneManager::LoadWithScreen(const std::string& sceneName) {
    s_loadWithScreenTarget = sceneName;

    auto* ui = UI::UIManager::GetInstance();
    if (ui) {
        ui->FadeIn(0.4f, []() {
            LoadScene("Loading");
        });
    }
    else {
        // Fallback: load directly if UI not available
        LoadScene(sceneName);
    }
}

void SceneManager::RenderActiveScene() {
    if (activeScene && activeScene->IsLoaded()) {
        activeScene->RenderInternal();
    }
}

void SceneManager::LoadFirstScene() {
    if (!sceneRegistry.empty()) {
        Scene* first = nullptr;
        for (auto& pair : sceneRegistry) {
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
    if (activeScene) {
        activeScene->UnloadInternal();
        activeScene = nullptr;
    }
}

void SceneManager::LoadSceneInternal(Scene* newScene) {
    if (!newScene) return;
    
    if (activeScene) {
        activeScene->UnloadInternal();
    }
    
    activeScene = newScene;
    activeScene->LoadInternal();
}

} // namespace Engine
