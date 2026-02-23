/**
 * @file main.cpp
 * @brief Main entry point for the 2.5D Engine
 */

// Windows includes
#include <crtdbg.h>

// Alpha Engine
#include "AEEngine.h"

// Engine Core
#include "Engine.hpp"

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
    AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, NULL);
    AESysSetWindowTitle("Escape From SIT");
	AESysSetWindowIcon("Assets/Brocsama.ico", 32, 32);

    // Initialize UI System
    UI::UIManager::Initialize();

    // Set default background color (Solid Black)
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

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
        
        // Update input (processes cursor centering when locked)
        Input::Update();
        
        // Check for exit conditions
        if (AEInputCheckTriggered(AEVK_ESCAPE) || !AESysDoesWindowExist()) {
            Application::Quit();
        }

        // Update scene (handles input, physics, rendering)
        SceneManager::UpdateActiveScene();

        // Update UI System
        if (UI::UIManager::GetInstance()) {
            UI::UIManager::GetInstance()->Update(Time::unscaledDeltaTime);
        }

        SceneManager::RenderActiveScene();

        // Render UI System on top
        if (UI::UIManager::GetInstance()) {
            UI::UIManager::GetInstance()->Render();
        }

        // Frame end
        AESysFrameEnd();
    }

    // Unload active scene first (ensures components clean up before managers)
    SceneManager::UnloadActiveScene();

    // Cleanup UI System
    UI::UIManager::Shutdown();

    // Cleanup Asset Manager
	AssetManager::UnloadAll();

    // Cleanup
    AESysExit();
    
    return 0;
}