/**
 * @file Scene.cpp
 * @brief Implementation of Scene class
 */
#include "Scene.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Physics/Physics2D.hpp"
#include <algorithm>

namespace Engine {

Scene::Scene(const std::string& name) : name(name) {}

Scene::~Scene() {
    UnloadInternal();
}

GameObject* Scene::CreateGameObject(const std::string& name) {
    auto obj = std::make_unique<GameObject>(name);
    GameObject* ptr = obj.get();
    ptr->scene = this;
    gameObjects.push_back(std::move(obj));
    return ptr;
}

GameObject* Scene::Find(const std::string& name) const {
    for (const auto& obj : gameObjects) {
        if (obj->name == name) return obj.get();
    }
    return nullptr;
}

const std::vector<std::unique_ptr<GameObject>>& Scene::GetGameObjects() const {
    return gameObjects;
}

void Scene::LoadInternal() {
    isLoaded = true;
    OnLoad();
}

void Scene::UnloadInternal() {
    if (isLoaded) {
        OnUnload();
        // Clear all objects
        gameObjects.clear();
        isLoaded = false;
    }
}

void Scene::UpdateInternal() {
    // Run physics simulation first
    Physics2D::FixedUpdate();
    
    // Fixed update for all GameObjects (physics components)
    for (auto& obj : gameObjects) {
        obj->InternalFixedUpdate();
    }
    
    // Update all GameObjects
    for (auto& obj : gameObjects) {
        obj->InternalUpdate();
    }
    
    OnUpdate();
    
    // Late update
    for (auto& obj : gameObjects) {
        obj->InternalLateUpdate();
    }
    
    // Process pending destructions
    GameObject::ProcessDestroyQueue();

    // Remove destroyed objects from the scene
    // This will trigger unique_ptr deletion and call the GameObject destructor
    gameObjects.erase(
        std::remove_if(gameObjects.begin(), gameObjects.end(), 
            [](const std::unique_ptr<GameObject>& obj) {
                return obj->isPendingDestruction;
            }),
        gameObjects.end()
    );
}

void Scene::RenderInternal() {
    if (!isLoaded) return;
	OnPreRender();
    
    // Apply camera settings if a main camera exists
    if (Camera::main) {
        Camera::main->Apply();
    }
    
    for (auto& obj : gameObjects) {
        obj->InternalRender();
    }

	OnAfterRender();
}

bool Scene::IsLoaded() const {
    return isLoaded;
}

} // namespace Engine
