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