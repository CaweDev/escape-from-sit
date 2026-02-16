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
#include <functional>

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

    /** Load a scene via the loading screen with fade transitions */
    static void LoadWithScreen(const std::string& sceneName);

    /** Queue a scene load for end of current update cycle */
    static void LoadSceneDeferred(const std::string& sceneName);

    /** Get the target scene name for loading screen transitions */
    static const std::string& GetLoadWithScreenTarget() { return s_loadWithScreenTarget; }
    
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

    /** Deferred scene load state */
    static std::string s_pendingScene;
    static bool s_hasPendingScene;

    /** Target scene for LoadWithScreen transitions */
    static std::string s_loadWithScreenTarget;
    
    /** Internal: actually switch scenes */
    static void LoadSceneInternal(Scene* newScene);
    
    SceneManager() = delete; // Static class
};

} // namespace Engine
