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
    /* Default fade durations */
	static f32 s_defaultLoadScreenFadeInDuration;  
	static f32 s_defaultLoadScreenFadeOutDuration;  

public:
    /** Get the default fade-in duration for loading screen transitions */
    static f32 GetDefaultFadeInDuration() { return s_defaultLoadScreenFadeInDuration; }

    /** Get the default fade-out duration for loading screen transitions */
    static f32 GetDefaultFadeOutDuration() { return s_defaultLoadScreenFadeOutDuration; }

    /** Load a scene by name (switches immediately) */
    static void LoadScene(const std::string& sceneName);
    
    /** Load a scene by build index (switches immediately) */
    static void LoadScene(int buildIndex);

    /** Load a scene via the loading screen with fade transitions (using configured defaults) */
    static void LoadWithLoadingScreen(const std::string& sceneName);

    /** Load a scene via the loading screen with explicit fade durations */
    static void LoadWithLoadingScreen(const std::string& sceneName, f32 fadeInDuration, f32 fadeOutDuration);

    /** Load a scene with a direct fade transition (using configured defaults) */
    static void LoadWithFade(const std::string& sceneName);

    /** Load a scene with a direct fade transition with explicit fade durations */
    static void LoadWithFade(const std::string& sceneName, f32 fadeInDuration, f32 fadeOutDuration);

    /** Queue a scene load for end of current update cycle */
    static void LoadSceneDeferred(const std::string& sceneName);

    /** Get the target scene name for loading screen transitions */
    static const std::string& GetLoadWithScreenTarget() { return s_loadWithScreenTarget; }

    /** Get the fade-in duration for the current loading screen transition */
    static f32 GetLoadWithScreenFadeInDuration() { return s_loadWithScreenFadeInDuration; }

    /** Get the fade-out duration for the current loading screen transition */
    static f32 GetLoadWithScreenFadeOutDuration() { return s_loadWithScreenFadeOutDuration; }

    /** Get the currently active scene */
    static Scene* GetActiveScene();
    
    /** Register a scene with the manager */
    static void RegisterScene(const std::string& name, Scene* scene);
    
    /** Get total number of registered scenes */
    static int GetSceneCount();

    /** Request a screen fade-out immediately after the next successful scene load */
    static void RequestFadeOutAfterNextSceneLoad(f32 duration);

    // Internal methods (called by engine)

    /** Update active scene */
    static void UpdateActiveScene();
    
    static void RenderActiveScene();

    /** Load first registered scene (call on startup) */
    static void LoadFirstScene();

    /** Unload active scene and cleanup all GameObjects (call before shutdown) */
    static void UnloadActiveScene();

private:
    static Scene* s_activeScene;
    static std::unordered_map<std::string, Scene*> s_sceneRegistry;

    /** Deferred scene load state */
    static std::string s_pendingScene;
    static bool s_hasPendingScene;

    /** Target scene for LoadWithScreen transitions */
    static std::string s_loadWithScreenTarget;

    /** Fade durations for LoadWithScreen transitions */
    static f32 s_loadWithScreenFadeInDuration;
    static f32 s_loadWithScreenFadeOutDuration;

    /** One-shot fade-out request applied after next LoadSceneInternal */
    static bool s_hasPendingPostLoadFadeOut;
    static f32 s_pendingPostLoadFadeOutDuration;

    /** Internal: actually switch scenes */
    static void LoadSceneInternal(Scene* newScene);
    
    SceneManager() = delete; // Static class
};

} // namespace Engine
