/**
 * @file Component.hpp
 * @brief Base component class with Unity-like lifecycle
 */
#pragma once
#include <string>

namespace Engine {

// Forward declarations
class GameObject;
class Transform;

/**
 * @class Component
 * @brief Base class for all components attached to GameObjects
 */
class Component {
public:
    std::string name;

    /** The GameObject this component is attached to */
    GameObject* gameObject = nullptr;
    
    /** Quick access to transform (cached reference) */
    Transform* transform = nullptr;
    
    /** Whether this component is enabled */
    bool enabled = true;

    Component() = default;
    virtual ~Component() = default;

    // Lifecycle methods (override in derived classes)
    
    /** Called when component is first created */
    virtual void Awake() {}
    
    /** Called before the first Update, after all Awake calls */
    virtual void Initialize() {}
    
    /** Called every frame */
    virtual void Update() {}
    
    /** Called every frame after all Update calls */
    virtual void LateUpdate() {}
    
    /** Called every physics step (fixed timestep) */
    virtual void FixedUpdate() {}
    
    /** Called when component is destroyed */
    virtual void OnDestroy() {}
    
    /** Called when component is enabled */
    virtual void OnEnable() {}
    
    /** Called when component is disabled */
    virtual void OnDisable() {}
    virtual void OnInspectorGUI() {}

protected:
    friend class GameObject;
    
    bool _started = false;
    bool _awoken = false;
};

/**
 * @brief Macro to require a component of type T on the same GameObject
 * This defines a static method that can be checked by GameObject during AddComponent
 */
#define REQUIRE_COMPONENT(Type) \
    using required_type = Type; \
    static const char* required_type_name() { return #Type; }

} // namespace Engine
