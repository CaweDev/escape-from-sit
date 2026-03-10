#include "DebugManager.hpp"
#include "DebugMenu.hpp"
#include "Engine/Input/Input.hpp"

namespace Game
{
    DebugMenu* DebugManager::s_menu = nullptr;
    bool DebugManager::s_wasCtrlTildePressed = false;

    void DebugManager::Initialize()
    {
        if (!s_menu)
        {
            s_menu = new DebugMenu();
            s_menu->Initialize();
        }
    }

    void DebugManager::Update()
    {
        if (!s_menu) return;

        if (Engine::Input::GetKeyDown(Engine::KeyCode::BackQuote))
        {
            s_menu->Toggle();
        }

        s_menu->Update();
    }

    void DebugManager::Shutdown()
    {
        if (s_menu)
        {
            s_menu->Shutdown();
            delete s_menu;
            s_menu = nullptr;
        }
    }
}
