/**
 * @file Scene.cpp
 * @brief Implementation of Scene class
 */
#include "Scene.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Physics/Physics2D.hpp"
#include "Engine/Graphics/IRenderer.hpp"
#include <algorithm>

namespace Engine {

Scene::Scene(const std::string& sceneName) : name(sceneName) {}

Scene::~Scene() {
    UnloadInternal();
}

/*!
 * \brief [AUTO] CreateGameObject
 * \param objectName [in]
 * \return 
 */
/*!
 * \brief [AUTO] CreateGameObject
 * \param objectName [in]
 * \return 
 */
GameObject* Scene::CreateGameObject(const std::string& objectName) {
    auto obj = std::make_unique<GameObject>(objectName);
    GameObject* ptr = obj.get();
    ptr->scene = this;
    if (isUpdating) {
        pendingGameObjects.push_back(std::move(obj));
    } else {
        gameObjects.push_back(std::move(obj));
    }
    return ptr;
}

/*!
 * \brief [AUTO] Find
 * \param objectName [in]
 * \return 
 */
/*!
 * \brief [AUTO] Find
 * \param objectName [in]
 * \return 
 */
GameObject* Scene::Find(const std::string& objectName) const {
    for (const auto& obj : gameObjects) {
        if (obj->name == objectName) return obj.get();
    }
    return nullptr;
}

/*!
 * \brief [AUTO] GetGameObjects
 * \return 
 */
/*!
 * \brief [AUTO] GetGameObjects
 * \return 
 */
const std::vector<std::unique_ptr<GameObject>>& Scene::GetGameObjects() const {
    return gameObjects;
}

/*!
 * \brief [AUTO] LoadInternal
 */
void Scene::LoadInternal() {
    isLoaded = true;
    OnLoad();
}

/*!
 * \brief [AUTO] UnloadInternal
 */
void Scene::UnloadInternal() {
    if (isLoaded) {
        OnUnload();
        // Clear all objects
        gameObjects.clear();
        isLoaded = false;
    }
}

/*!
 * \brief [AUTO] UpdateInternal
 */
void Scene::UpdateInternal() {
    // Run physics simulation first
    Physics2D::FixedUpdate();
    
    isUpdating = true;

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

    isUpdating = false;

    // Flush objects created during the update loop
    for (auto& obj : pendingGameObjects) {
        gameObjects.push_back(std::move(obj));
    }
    pendingGameObjects.clear();
    
    // Process pending destructions
    GameObject::ProcessDestroyQueue();

    // Remove destroyed objects from the scene
    gameObjects.erase(
        std::remove_if(gameObjects.begin(), gameObjects.end(), 
            [](const std::unique_ptr<GameObject>& obj) {
                return obj->isPendingDestruction;
            }),
        gameObjects.end()
    );
}

/*!
 * \brief [AUTO] RenderInternal
 */
void Scene::RenderInternal() {
    if (!isLoaded) return;
	OnPreRender();
    
    // Apply camera settings if a main camera exists
    if (Camera::main) {
        Camera::main->Apply();
    }
    
    // Collect all renderers from active objects
    std::vector<IRenderer*> renderers;
    // Simple heuristic reservation
    renderers.reserve(gameObjects.size()); 

    for (auto& obj : gameObjects) {
        if (!obj->active) continue;
        
        for (auto& comp : obj->GetComponents<IRenderer>()) {
            if (comp->enabled) {
                renderers.push_back(comp);
  
            }
        }
    }
    
    // Sort renderers by sorting key (Descending order: farthest first/Painter's Algorithm)
    std::sort(renderers.begin(), renderers.end(), [](IRenderer* a, IRenderer* b) {
        return a->GetSortingKey() > b->GetSortingKey();
    });
    
    // Render sorted list
    for (auto* renderer : renderers) {
        renderer->Render();
    }

	OnAfterRender();
    Physics2D::DrawDebugView();
}

/*!
 * \brief [AUTO] IsLoaded
 * \return 
 */
/*!
 * \brief [AUTO] IsLoaded
 * \return 
 */
bool Scene::IsLoaded() const {
    return isLoaded;
}

} // namespace Engine
