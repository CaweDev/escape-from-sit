/**
 * @file EditorManager.cpp
 * @brief EditorManager implementation
 */

#include "EditorManager.hpp"
#include "GameEditor.hpp"

// Alpha Engine
#include "AEEngine.h"

// ImGui
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
namespace AEEditor
{

    // Local State
    static WNDPROC gOriginalWndProc = nullptr;
    static bool gDockInitialized = false;
    static HWND gEditorHwnd = nullptr;

    static float gCamOffsetX = 0.0f;
    static float gCamOffsetY = 0.0f;

    static HANDLE gActiveGameProcess = NULL;
    static bool gIsExternalGameRunning = false;

    static LRESULT CALLBACK EditorWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui::GetCurrentContext() != nullptr)
        {
            if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
                return true;
        }

        // Execute original engine window procedure if valid
        if (gOriginalWndProc != nullptr)
        {
            return CallWindowProc(gOriginalWndProc, hWnd, msg, wParam, lParam);
        }

        // Failsafe OS routing
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    void EditorManager::Initialize(HWND hwnd)
    {
        gEditorHwnd = hwnd;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // Initialize backends FIRST to allocate internal memory structs
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplOpenGL3_Init("#version 130");

        // Install OS message hook LAST to guarantee safe memory access
        gOriginalWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)EditorWndProc);
    }

    void EditorManager::Exit()
    {
        // Retrieve handle to detach hook
        HWND hwnd = AESysGetWindowHandle();
        if (gEditorHwnd != nullptr && gOriginalWndProc != nullptr)
        {
            SetWindowLongPtr(gEditorHwnd, GWLP_WNDPROC, (LONG_PTR)gOriginalWndProc);
            gOriginalWndProc = nullptr;
            gEditorHwnd = nullptr;
        }

        if (gOriginalWndProc != nullptr)
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)gOriginalWndProc);
            gOriginalWndProc = nullptr;
        }

        if (gActiveGameProcess != NULL)
        {
            CloseHandle(gActiveGameProcess);
            gActiveGameProcess = NULL;
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        GameEditor::Exit();
        ImGui::DestroyContext();
    }

    bool EditorManager::CheckExternalProcess(HWND hwnd)
    {
        if (!gIsExternalGameRunning)
        {
            return false;
        }

        if (WaitForSingleObject(gActiveGameProcess, 0) == WAIT_OBJECT_0)
        {
            CloseHandle(gActiveGameProcess);
            gActiveGameProcess = NULL;
            gIsExternalGameRunning = false;

            ShowWindow(hwnd, SW_SHOW);
            return false;
        }

        return true;
    }

    void EditorManager::BuildUI(HWND hwnd)
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
                                        ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoDocking |
                                        ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::Begin("EditorShell", nullptr, window_flags);

        ImGuiID ds_id = ImGui::GetID("EditorDockSpace");
        ImGui::DockSpace(ds_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        if (!gDockInitialized)
        {
            ImGui::DockBuilderRemoveNode(ds_id);
            ImGui::DockBuilderAddNode(ds_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(ds_id, ImGui::GetMainViewport()->WorkSize);
            ImGuiID left = ImGui::DockBuilderSplitNode(ds_id, ImGuiDir_Left, 0.2f, nullptr, &ds_id);
            ImGuiID right = ImGui::DockBuilderSplitNode(ds_id, ImGuiDir_Right, 0.2f, nullptr, &ds_id);
            ImGuiID top_left = ImGui::DockBuilderSplitNode(left, ImGuiDir_Up, 0.5f, nullptr, &left);
            ImGui::DockBuilderDockWindow("Toolbar", top_left);
            ImGui::DockBuilderDockWindow("Hierarchy", left);
            ImGui::DockBuilderDockWindow("Inspector", right);
            ImGui::DockBuilderDockWindow("Scene", ds_id);
            ImGui::DockBuilderFinish(ds_id);
            gDockInitialized = true;
        }
        ImGui::End();
        ImGui::PopStyleColor();

        // 1. Render Toolbar / Drag Source
        GameEditor::DrawAssetBrowser();

        ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoScrollbar);
        if (ImGui::Button("Play External", ImVec2(248, 40)))
        {
            GameEditor::SaveMap("Assets/tmpMap.txt");

            char exePath[MAX_PATH];
            GetModuleFileNameA(NULL, exePath, MAX_PATH);

            char cmdArgs[MAX_PATH * 2];
            sprintf_s(cmdArgs, "\"%s\" -game", exePath);

            // Specifies the window station, desktop, standard handles, and appearance of the main window for the new process. Zero-initialized via struct size assignment per Win32 specification.
            STARTUPINFOA si = {sizeof(si)};
            // Output struct populated by the OS. Receives the globally unique identifiers and handles for the newly allocated process and its primary thread.
            PROCESS_INFORMATION pi;

            // launch the game exe file
            if (CreateProcessA(NULL, cmdArgs, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            {
                gActiveGameProcess = pi.hProcess;
                // stop editor
                gIsExternalGameRunning = true;
                // thread stuff
                CloseHandle(pi.hThread);
                // hide the editor window so that the game can be full screened
                ShowWindow(hwnd, SW_HIDE);
            }
        }
        ImGui::End();

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImVec2 winCenter = {winPos.x + winSize.x * 0.5f, winPos.y + winSize.y * 0.5f};
        ImVec2 scrCenter = {(float)AEGfxGetWindowWidth() * 0.5f, (float)AEGfxGetWindowHeight() * 0.5f};

        gCamOffsetX = winCenter.x - scrCenter.x;
        gCamOffsetY = scrCenter.y - winCenter.y;

        GameEditor::DrawMapEditorViewport();

        ImGui::End();
        ImGui::PopStyleColor();

        ImGui::Begin("Hierarchy");
        ImGui::Text("TBC");
        ImGui::End();

        ImGui::Begin("Inspector");
        ImGui::Text("TBC");
        ImGui::End();
    }

    void EditorManager::RenderUI()
    {

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    float EditorManager::GetCamOffsetX() { return gCamOffsetX; }
    float EditorManager::GetCamOffsetY() { return gCamOffsetY; }
}