/*!
 * \file Inventory.cpp
 * \brief Implementation of a small item inventory component
 */
#include "Inventory.hpp"
#include "Game/Items/Item.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Input/Input.hpp"

namespace Game
{

    /*! \brief Handle input to switch active slot or use item. */
    void Inventory::Update()
    {
        if (Engine::Input::GetKeyDown(Engine::KeyCode::Alpha1))
            activeSlot = 0;
        if (Engine::Input::GetKeyDown(Engine::KeyCode::Alpha2))
            activeSlot = 1;
        if (Engine::Input::GetKeyDown(Engine::KeyCode::Alpha3))
            activeSlot = 2;

        if (Engine::Input::GetKeyDown(Engine::KeyCode::F))
        {
            UseActiveItem();
        }
    }

    /*! \brief Add an item to the first free slot. \param itemObj [in] Item GameObject \return true if added */
    bool Inventory::AddItem(Engine::GameObject *itemObj)
    {
        for (int i = 0; i < MAX_SLOTS; ++i)
        {
            if (!slots[i])
            {
                slots[i] = itemObj;
                itemObj->active = false;
                return true;
            }
        }
        return false;
    }

    /*! \brief Use the currently active item and clear its slot. */
    void Inventory::UseActiveItem()
    {
        Engine::GameObject *itemObj = slots[activeSlot];
        if (!itemObj)
            return;

        auto usable = itemObj->GetComponent<Item>();
        if (usable)
        {
            usable->Use(gameObject);
        }

        slots[activeSlot] = nullptr;
    }

}
