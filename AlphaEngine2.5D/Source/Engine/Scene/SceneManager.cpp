/**
 * @file SceneManager.cpp
 * @brief Implementation of SceneManager class
 */
#include "SceneManager.hpp"

namespace Engine {

Scene* SceneManager::activeScene = nullptr;
std::unordered_map<std::string, Scene*> SceneManager::sceneRegistry;

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

void SceneManager::LoadSceneInternal(Scene* newScene) {
    if (!newScene) return;
    
    if (activeScene) {
        activeScene->UnloadInternal();
    }
    
    activeScene = newScene;
    activeScene->LoadInternal();
}

} // namespace Engine
