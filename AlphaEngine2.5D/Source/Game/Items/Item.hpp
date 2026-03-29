/*!
 * \file Item.hpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief Base class for interactive items in the world
 */

#pragma once
#include <string>
#include "Engine/Core/Component.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <cmath>

#include "Engine/Interaction/Interactable.hpp"

namespace Game
{

    /*!
     * \class Item
     * \brief Abstract base component for game items that can be used or collected
     */
    class Item : public Engine::Component
    {
    public:
        Engine::Interactable *interactable = nullptr;

        std::string itemName = "Unknown";
        /*! \brief Initialize item component and set up interactions. */
        void Initialize() override;

        /*! \brief Use the item by a GameObject (player, NPC). \param user [in] user GameObject */
        virtual void Use(Engine::GameObject *user) = 0;

    private:
        Item *itemData = nullptr;

        /*! \brief Called when item is collected. */
        void OnCollect();
    };

}
