/*!
 * \file AlphaEngine2.5D\Source\Game\Debugger\DebugManager.hpp
 * \brief DebugManager implementation that manages the debug menu and its input handling
 */

#pragma once

namespace Game
{
    class DebugMenu;

    class DebugManager
    {
    public:
        static void Initialize();
        static void Update();
        static void Shutdown();

        static DebugMenu* GetMenu() { return s_menu; }
        
    private:
        static DebugMenu* s_menu;
        
        static bool s_wasCtrlTildePressed;
    };
}
