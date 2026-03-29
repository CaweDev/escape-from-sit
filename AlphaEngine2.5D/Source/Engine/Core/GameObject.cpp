/**
 * @file GameObject.cpp
 * @brief Implementation of GameObject class
 */
#include "GameObject.hpp"
#include <algorithm>

namespace Engine {

std::vector<GameObject*> GameObject::allGameObjects;
std::vector<GameObject*> GameObject::objectsToDestroy;

GameObject::GameObject(const std::string& name) : name(name) {
    // Every GameObject has a Transform
    transform = AddComponent<Transform>();
    
    // Register in global list
    allGameObjects.push_back(this);
}

GameObject::~GameObject() {
    // Remove from global list
    allGameObjects.erase(
        std::remove(allGameObjects.begin(), allGameObjects.end(), this),
        allGameObjects.end()
    );
    
    // Destroy all components
    for (auto& comp : components) {
        comp->OnDestroy();
    }
    components.clear();
}

/*!
 * \brief [AUTO] Find
 * \param name [in]
 * \return 
 */
GameObject* GameObject::Find(const std::string& name) {
    for (auto* obj : allGameObjects) {
        if (obj->name == name) return obj;
    }
    return nullptr;
}

/*!
 * \brief [AUTO] FindGameObjectsWithTag
 * \param tag [in]
 * \return 
 */
std::vector<GameObject*> GameObject::FindGameObjectsWithTag(const std::string& tag) {
    std::vector<GameObject*> result;
    for (auto* obj : allGameObjects) {
        if (obj->tag == tag) result.push_back(obj);
    }
    return result;
}

/*!
 * \brief [AUTO] Destroy
 * \param obj [in]
 */
void GameObject::Destroy(GameObject* obj) {
    if (obj) {
        obj->isPendingDestruction = true;
        objectsToDestroy.push_back(obj);
    }
}

/*!
 * \brief [AUTO] ProcessDestroyQueue
 */
void GameObject::ProcessDestroyQueue() {
    // We no longer delete here. 
    // The Scene will remove the object from its list, which triggers unique_ptr cleanup.
    objectsToDestroy.clear();
}

/*!
 * \brief [AUTO] GetAllGameObjects
 * \return 
 */
const std::vector<GameObject*>& GameObject::GetAllGameObjects() {
    return allGameObjects;
}

/*!
 * \brief [AUTO] InternalUpdate
 */
void GameObject::InternalUpdate() {
    if (!active || isPendingDestruction) return;
    
    // Use index-based iteration to avoid iterator invalidation
    for (size_t i = 0; i < components.size(); ++i) {
        auto& comp = components[i];
        if (!comp || !comp->enabled) continue;
        
        // Call Start on first frame
        if (!comp->_started) {
            comp->Initialize();
            comp->_started = true;
        }
        
        comp->Update();
        
        // Check if destroyed during update
        if (isPendingDestruction) return;
    }
}

/*!
 * \brief [AUTO] InternalLateUpdate
 */
void GameObject::InternalLateUpdate() {
    if (!active || isPendingDestruction) return;
    
    // Use index-based iteration to avoid iterator invalidation
    for (size_t i = 0; i < components.size(); ++i) {
        auto& comp = components[i];
        if (comp && comp->enabled) {
            comp->LateUpdate();
            
            // Check if destroyed during late update
            if (isPendingDestruction) return;
        }
    }
}

/*!
 * \brief [AUTO] InternalFixedUpdate
 */
void GameObject::InternalFixedUpdate() {
    if (!active || isPendingDestruction) return;
    
    // Use index-based iteration to avoid iterator invalidation
    for (size_t i = 0; i < components.size(); ++i) {
        auto& comp = components[i];
        if (comp && comp->enabled) {
            comp->FixedUpdate();
            
            // Check if destroyed during fixed update
            if (isPendingDestruction) return;
        }
    }
}

/*!
 * \brief [AUTO] InternalRender
 */
void GameObject::InternalRender() {
if (!active || isPendingDestruction) return;
    
// Find and sort all renderers
std::vector<IRenderer*> renderers;
// Use index-based iteration to avoid iterator invalidation
for (size_t i = 0; i < components.size(); ++i) {
    auto& comp = components[i];
    if (comp && comp->enabled) {
        auto* renderer = dynamic_cast<IRenderer*>(comp.get());
        if (renderer) renderers.push_back(renderer);
    }
}
    
    // Sort by sorting key
    std::sort(renderers.begin(), renderers.end(), [](auto* a, auto* b) {
        return a->GetSortingKey() < b->GetSortingKey();
    });
    
    for (auto* renderer : renderers) {
        renderer->Render();
    }
}

} // namespace Engine
