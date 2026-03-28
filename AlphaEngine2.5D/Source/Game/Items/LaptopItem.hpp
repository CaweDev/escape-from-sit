#pragma once
#include "Item.hpp"

namespace Game
{

    class LaptopItem : public Item
    {
    public:
        void Update() override;
        void Use(Engine::GameObject *user) override;
        float fuseTimer = 1.5f;

    private:
        bool isThrown = false;
    };

}