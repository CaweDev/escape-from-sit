/*!
 * \file AlphaEngine2.5D\Source\Game\Items\SodaCanItem.hpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief [AUTO] Added Doxygen file header
 */

#pragma once
#include "Item.hpp"

namespace Game
{

    class SodaCanItem: public Item
    {
    public:
        void Use(Engine::GameObject *user) override;
    };

}