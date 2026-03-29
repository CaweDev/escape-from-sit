/*!
 * \file AlphaEngine2.5D\Source\Game\Debugger\DebugManager.cpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief [AUTO] Added Doxygen file header
 */

#include "DebugManager.hpp"
#include "DebugMenu.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Core/ConfigManager.hpp"

namespace
{
    struct DebugManagerConfig : public Engine::IConfig
    {
        bool enabled = true;
        int toggleKey = static_cast<int>(Engine::KeyCode::BackQuote);

        void FromJson(const Json::Value& j) override
        {
            enabled = j.get("enabled", enabled).asBool();
            toggleKey = j.get("toggleKey", toggleKey).asInt();
        }
    };

    DebugManagerConfig g_debugManagerConfig;
    bool g_debugManagerConfigLoaded = false;

    const DebugManagerConfig& GetDebugManagerConfig()
    {
        if (!g_debugManagerConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/debug_manager.json", g_debugManagerConfig);
            g_debugManagerConfigLoaded = true;
        }
        return g_debugManagerConfig;
    }
}

namespace Game
{
    DebugMenu* DebugManager::s_menu = nullptr;
    bool DebugManager::s_wasCtrlTildePressed = false;

    void DebugManager::Initialize()
    {
        if (!GetDebugManagerConfig().enabled) return;

        if (!s_menu)
        {
            s_menu = new DebugMenu();
            s_menu->Initialize();
        }
    }

    void DebugManager::Update()
    {
        if (!GetDebugManagerConfig().enabled) return;
        if (!s_menu) return;

        if (Engine::Input::GetKeyDown(static_cast<Engine::KeyCode>(GetDebugManagerConfig().toggleKey)))
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
