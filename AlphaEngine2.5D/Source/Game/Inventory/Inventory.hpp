/*!
 * \file Inventory.hpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief Simple player inventory component with limited slots
 */

#pragma once
#include "Engine/Core/Component.hpp"

namespace Engine
{
    class GameObject;
}

namespace Game
{

    /*! \class Inventory
     *  \brief Holds a small number of usable items for the player
     */
    class Inventory : public Engine::Component
    {
    public:
        static const int MAX_SLOTS = 3;
        Engine::GameObject *slots[MAX_SLOTS] = {nullptr, nullptr, nullptr};
        int activeSlot = 0;

        /*! \brief Per-frame update (input or slot management). */
        void Update() override;
        /*! \brief Try to add an item to inventory. \param itemObj [in] item GameObject \return true if added */
        bool AddItem(Engine::GameObject *itemObj);
        /*! \brief Use the currently active item. */
        void UseActiveItem();
    };

}
