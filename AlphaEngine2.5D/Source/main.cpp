/**
 * @file main.cpp
 * @brief Main entry point for the 2.5D Engine
 */

// Windows includes
#include <crtdbg.h>

// Alpha Engine
#include "AEEngine.h"

// Engine Core
#include "Engine/Core/Application.hpp"
#include "Engine/Core/Time.hpp"

// Engine Scene
#include "Engine/Scene/SceneManager.hpp"

// Game Scenes
#include "Game/Scenes/SceneRegistry.hpp"

using namespace Engine;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize Alpha Engine
    AESysInit(hInstance, nCmdShow, 1280, 720, 1, 60, false, NULL);
    AESysSetWindowTitle("Totally not Unity Engine. It's Alpha Engine!!!");

    // Register all scenes and load the first one
    Game::RegisterAllScenes();
    SceneManager::LoadFirstScene(); // Load the first scene in SceneRegistry

    // Main game loop
    while (Application::isRunning)
    {
        // Frame start
        AESysFrameStart();
        
        // Update time
        Time::Update();
        
        // Check for exit conditions
        if (AEInputCheckTriggered(AEVK_ESCAPE) || !AESysDoesWindowExist()) {
            Application::Quit();
        }

        // Update scene (handles input, physics, rendering)
        SceneManager::UpdateActiveScene();

        SceneManager::RenderActiveScene();

        // Frame end
        AESysFrameEnd();
    }

    // Cleanup
    AESysExit();
    
    return 0;
}