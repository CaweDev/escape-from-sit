/**
 * @file EditorManager.hpp
 * @brief Editor layout and state management
 */
#pragma once

#include <windows.h>
namespace AEEditor
{
    class EditorManager
    {
    public:
        static void Initialize(HWND hwnd);
        static void Exit();

        static bool CheckExternalProcess(HWND hwnd);
        static void BuildUI(HWND hwnd);
        static void RenderUI();

        static float GetCamOffsetX();
        static float GetCamOffsetY();
    };
}