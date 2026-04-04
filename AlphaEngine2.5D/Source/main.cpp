/**
 * @file main.cpp
 * @brief Main entry point for the 2.5D Engine
 */

#ifdef _WIN32
extern "C"
{
    // Tell NVIDIA Optimus to use the high-performance GPU
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;

    // Tell AMD Switchable Graphics to use the high-performance GPU
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

// Windows includes
#include <windows.h>
#include <crtdbg.h>

// Alpha Engine
#include "AEEngine.h"

// Engine Core
#include "Engine.hpp"

// Texture static shutdown
#include "Engine/Graphics/Texture2D.hpp"

// Engine subsystem config (single call initialises all config files)
#include "Engine/Core/ConfigManager.hpp"

// Game Scenes
#include "Game/Scenes/SceneRegistry.hpp"

// Debug
#include "Game/Debugger/DebugManager.hpp"

#include "Editor/Index.hpp"
// Game systems
#include "Game/Puzzle/PuzzleManager.hpp"
#include "Editor/ReflectionRegistry.hpp"

using namespace Engine;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    UNREFERENCED_PARAMETER(hPrevInstance);
    //UNREFERENCED_PARAMETER(lpCmdLine);

    bool isEditorMode = (wcsstr(lpCmdLine, L"-editor") != nullptr);
    // `-editor` switches boot path to EditorScene + editor UI loop.

    // Initialize Alpha Engine
    AESysInit(hInstance, nCmdShow, 1600, 900, 0, 60, false, NULL);
    AESysSetWindowTitle("Escape From SIT");
    AESysSetWindowIcon("Assets/Brocsama.ico", 32, 32);

    HWND hwnd = AESysGetWindowHandle();

    HICON largeIcon = static_cast<HICON>(LoadImageW(nullptr,
        L"Assets\\Brocsama.ico",
        IMAGE_ICON,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON),
        LR_LOADFROMFILE)); // Used for taskbar/Alt+Tab surfaces.

    HICON smallIcon = static_cast<HICON>(LoadImageW(nullptr,
        L"Assets\\Brocsama.ico",
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_LOADFROMFILE)); // Used for title bar/small shell surfaces.

    if (hwnd)
    {
		if (largeIcon) // Set Big Icon (Taskbar/Alt+Tab)
        {
            SendMessage(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(largeIcon));
            SetClassLongPtr(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(largeIcon));
        }

		if (smallIcon) // Set Small Icon (Title Bar)
        {
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
            SetClassLongPtr(hwnd, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(smallIcon));
        }
    }

    if (isEditorMode)
    {
        // Editor host must be initialized before scene/game systems start.
        AEEditor::EditorManager::Initialize(hwnd);
    }

    AEEditor::ReflectionRegistry::InitializeMetaRegistry();

    // Initialize Sound Manager
    SoundManager::Initialize();

    // Initialize all engine subsystems from config files
    ConfigManager::InitializeAll();

    // Initialize UI System
    UI::UIManager::Initialize();

    // Initialize Debug Manager
    Game::DebugManager::Initialize();

    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // Register all scenes and load the first one
    Game::RegisterAllScenes();

    if (isEditorMode)
    {
        SceneManager::LoadScene("EditorScene");
    }
    else
    {
        SceneManager::LoadFirstScene();
    }

    // Main game loop
    while (Application::isRunning)
    {
        // Frame start
        AESysFrameStart();

        // Update time
        Time::Update();

        // Update input (processes cursor centering when locked)
        Input::Update();

        // Update audio fades/transitions
        SoundManager::Update(Time::unscaledDeltaTime);

        // Check for exit conditions
        if (!AESysDoesWindowExist())
        {
            Application::Quit();
        }

        if (isEditorMode)
        {
            // Skip gameplay updates while the external editor process owns control.
            if (AEEditor::EditorManager::CheckExternalProcess(hwnd))
            {
                AESysFrameEnd();
                continue;
            }

            if (AEEditor::GameEditor::Is3DMode())
            {
                SceneManager::UpdateActiveScene();
            }

            AEEditor::EditorManager::BuildUI(hwnd);

            if (AEEditor::GameEditor::Is3DMode())
            {

                SceneManager::RenderActiveScene();
            }

            AEEditor::EditorManager::RenderUI();
            // Keep cursor unlocked while editing so ImGui/editor widgets remain usable.
            Input::RegisterCursorUnlock("EditorMode");
        }
        else
        {
            // Update scene (handles input, physics, rendering)
            SceneManager::UpdateActiveScene();

            // Update UI System
            if (UI::UIManager::GetInstance())
            {
                UI::UIManager::GetInstance()->Update(Time::unscaledDeltaTime);
            }

            Game::DebugManager::Update();

            SceneManager::RenderActiveScene();

            // Render UI System on top
            if (UI::UIManager::GetInstance())
            {
                UI::UIManager::GetInstance()->Render();
            }

            // Frame end
        }
        AESysFrameEnd();
    }

    // Unload active scene first (ensures components clean up before managers)
    SceneManager::UnloadActiveScene();

    // Cleanup static game data
    Game::PuzzleManager::FreeManager();

    // Cleanup Debug Manager
    Game::DebugManager::Shutdown();

    // Cleanup UI System
    UI::UIManager::Shutdown();

    // Cleanup Sound Manager
    SoundManager::Shutdown();

    // Cleanup Asset Manager
    AssetManager::UnloadAll();

    if (isEditorMode)
    {
        AEEditor::EditorManager::Exit();
    }

    // Cleanup
    AESysExit();

    if (smallIcon)
    {
        DestroyIcon(smallIcon);
    }

    if (largeIcon)
    {
        DestroyIcon(largeIcon);
    }

    return 0;
}