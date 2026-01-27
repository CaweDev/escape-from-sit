/**
 * @file SceneManager.hpp
 * @brief Static scene management class
 */
#pragma once

#include "Scene.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Engine {

/**
 * @class SceneManager
 * @brief Static class for loading and managing scenes
 */
class SceneManager {
public:
    /** Load a scene by name (switches immediately) */
    static void LoadScene(const std::string& sceneName);
    
    /** Load a scene by build index (switches immediately) */
    static void LoadScene(int buildIndex);
    
    /** Get the currently active scene */
    static Scene* GetActiveScene();
    
    /** Register a scene with the manager */
    static void RegisterScene(const std::string& name, Scene* scene);
    
    /** Get total number of registered scenes */
    static int GetSceneCount();

    // Internal methods (called by engine)

    /** Update active scene */
    static void UpdateActiveScene();
    
    static void RenderActiveScene();

    /** Load first registered scene (call on startup) */
    static void LoadFirstScene();

    /** Unload active scene and cleanup all GameObjects (call before shutdown) */
    static void UnloadActiveScene();

private:
    static Scene* activeScene;
    static std::unordered_map<std::string, Scene*> sceneRegistry;
    
    /** Internal: actually switch scenes */
    static void LoadSceneInternal(Scene* newScene);
    
    SceneManager() = delete; // Static class
};

} // namespace Engine
