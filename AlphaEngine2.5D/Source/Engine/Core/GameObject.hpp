/**
 * @file GameObject.hpp
 * @brief GameObject class with component management
 */
#pragma once

#include "Component.hpp"
#include "Transform.hpp"
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <cstdio>
#include "../Graphics/IRenderer.hpp"

namespace Engine {

/**
 * @class GameObject
 * @brief Container for components, similar to Unity's GameObject
 */
class GameObject {
public:
    /** Name of the GameObject */
    std::string name;
    
    /** Tag for identification */
    std::string tag;
    
    /** Whether the GameObject is active */
    bool active = true;
    
    /** Transform component (always present) */
    Transform* transform = nullptr;

    // Constructors
    
    explicit GameObject(const std::string& name = "GameObject");
    ~GameObject();

    // Component management
    
    /** Helper to check if T has required_type alias */
    template<typename T, typename = void>
    struct has_requirement : std::false_type {};

    template<typename T>
    struct has_requirement<T, std::void_t<typename T::required_type>> : std::true_type {};

    /** Add a component of type T */
    template<typename T>
    T* AddComponent() {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        
        auto component = std::make_unique<T>();
        T* ptr = component.get();
        
        ptr->gameObject = this;
        
        // Check requirements if they exist
        if constexpr (has_requirement<T>::value) {
            if (!this->HasComponent<typename T::required_type>()) {
                printf("[Warning] Component %s requires %s on GameObject %s!\n", typeid(T).name(), T::required_type_name(), name.c_str());
            }
        }
        
        // Special handling for Transform - it references itself
        if constexpr (std::is_same<T, Transform>::value) {
            ptr->transform = ptr;
        } else {
            ptr->transform = this->transform;
        }
        
        components.push_back(std::move(component));
        
        // Call Awake immediately
        ptr->Awake();
        ptr->_awoken = true;
        
        return ptr;
    }
    
    /** Get a component of type T */
    template<typename T>
    T* GetComponent() const {
        for (const auto& comp : components) {
            T* casted = dynamic_cast<T*>(comp.get());
            if (casted) return casted;
        }
        return nullptr;
    }
    
    /** Get all components of type T */
    template<typename T>
    std::vector<T*> GetComponents() const {
        std::vector<T*> result;
        for (const auto& comp : components) {
            T* casted = dynamic_cast<T*>(comp.get());
            if (casted) result.push_back(casted);
        }
        return result;
    }
    
    /** Remove first component of type T */
    template<typename T>
    void RemoveComponent() {
        for (auto it = components.begin(); it != components.end(); ++it) {
            T* casted = dynamic_cast<T*>(it->get());
            if (casted && casted != transform) { // Never remove Transform
                casted->OnDestroy();
                components.erase(it);
                return;
            }
        }
    }
    
    /** Check if has component of type T */
    template<typename T>
    bool HasComponent() const {
        return GetComponent<T>() != nullptr;
    }

    // Static methods
    
    /** Find GameObject by name */
    static GameObject* Find(const std::string& name);
    
    /** Find all GameObjects with tag */
    static std::vector<GameObject*> FindGameObjectsWithTag(const std::string& tag);
    
    /** Destroy a GameObject */
    static void Destroy(GameObject* obj);
    
    /** Process pending destructions (called by engine) */
    static void ProcessDestroyQueue();
    
    /** Get all GameObjects */
    static const std::vector<GameObject*>& GetAllGameObjects();

    // Internal: Called by engine each frame
    
    void InternalUpdate();
    void InternalLateUpdate();
    void InternalFixedUpdate();
    void InternalRender();

private:
    std::vector<std::unique_ptr<Component>> components;
    
    // Global tracking
    static std::vector<GameObject*> allGameObjects;
    static std::vector<GameObject*> objectsToDestroy;
};

} // namespace Engine
