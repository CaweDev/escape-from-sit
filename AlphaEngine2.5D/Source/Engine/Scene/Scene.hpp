/**
 * @file Scene.hpp
 * @brief Scene class for containing GameObjects
 */
#pragma once

#include "../Core/GameObject.hpp"
#include <string>
#include <vector>
#include <memory>

namespace Engine {

/**
 * @class Scene
 * @brief Container for GameObjects with lifecycle methods
 */
class Scene {
public:
    /** Name of the scene */
    std::string name;
    
    /** Build index for scene ordering */
    int buildIndex = -1;

    Scene(const std::string& name = "Untitled");
    virtual ~Scene();

    // Lifecycle (override in derived classes)
    
    /** Called when scene starts loading */
    virtual void OnLoad() {}
    
    /** Called when scene is about to unload */
    virtual void OnUnload() {}
    
    /** Called every frame while scene is active */
    virtual void OnUpdate() {}

    /** Called every frame before scene is about to render */
    virtual void OnPreRender() {}

    /** Called every frame after scene renders gameobjects */
    virtual void OnAfterRender() {}

    // GameObject management
    
    /** Create a new GameObject in this scene */
    GameObject* CreateGameObject(const std::string& name = "GameObject");
    
    /** Find GameObject by name in this scene */
    GameObject* Find(const std::string& name) const;
    
    /** Get all GameObjects in this scene */
    const std::vector<std::unique_ptr<GameObject>>& GetGameObjects() const;

    // Internal methods (called by SceneManager)
    
    void LoadInternal();
    
    void UnloadInternal();
    
    void UpdateInternal();
    
    void RenderInternal();

    bool IsLoaded() const;

protected:
    std::vector<std::unique_ptr<GameObject>> gameObjects;
    std::vector<std::unique_ptr<GameObject>> pendingGameObjects;
    bool isLoaded = false;
    bool isUpdating = false;
};

} // namespace Engine
