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

GameObject* GameObject::Find(const std::string& name) {
    for (auto* obj : allGameObjects) {
        if (obj->name == name) return obj;
    }
    return nullptr;
}

std::vector<GameObject*> GameObject::FindGameObjectsWithTag(const std::string& tag) {
    std::vector<GameObject*> result;
    for (auto* obj : allGameObjects) {
        if (obj->tag == tag) result.push_back(obj);
    }
    return result;
}

void GameObject::Destroy(GameObject* obj) {
    if (obj) {
        objectsToDestroy.push_back(obj);
    }
}

void GameObject::ProcessDestroyQueue() {
    for (auto* obj : objectsToDestroy) {
        delete obj;
    }
    objectsToDestroy.clear();
}

const std::vector<GameObject*>& GameObject::GetAllGameObjects() {
    return allGameObjects;
}

void GameObject::InternalUpdate() {
    if (!active) return;
    
    for (auto& comp : components) {
        if (!comp->enabled) continue;
        
        // Call Start on first frame
        if (!comp->_started) {
            comp->Initialize();
            comp->_started = true;
        }
        
        comp->Update();
    }
}

void GameObject::InternalLateUpdate() {
    if (!active) return;
    
    for (auto& comp : components) {
        if (comp->enabled) {
            comp->LateUpdate();
        }
    }
}

void GameObject::InternalFixedUpdate() {
    if (!active) return;
    
    for (auto& comp : components) {
        if (comp->enabled) {
            comp->FixedUpdate();
        }
    }
}

void GameObject::InternalRender() {
    if (!active) return;
    
    // Find and sort all renderers
    std::vector<IRenderer*> renderers;
    for (auto& comp : components) {
        if (comp->enabled) {
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
