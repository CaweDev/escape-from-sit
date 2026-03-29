/*!
 * \file LaptopItem.hpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief Header for a throwable laptop item that can distract NPCs
 */

#pragma once
#include "Item.hpp"
#include <vector>

namespace Engine
{
    class Rigidbody2D;
}

namespace Game
{

    /*! \class LaptopItem
     *  \brief Throwable item that can distract NPCs when it lands
     */
    class LaptopItem : public Item
    {
    public:
        /*! \brief Initialize physics/sprite references. */
        void Initialize() override;
        /*! \brief Per-frame update for thrown motion and timers. */
        void Update() override;
        /*! \brief Cleanup state on destroy. */
        void OnDestroy() override;
        /*! \brief Use the item (throw or activate). \param user [in] user GameObject */
        void Use(Engine::GameObject *user) override;
        float timer = 1.5f;

    private:
        bool isThrown = false;
        bool hasLanded = false;
        Engine::Rigidbody2D* rigidbody2D;
        std::vector<Engine::GameObject *> distractedEntities;
    };

}
