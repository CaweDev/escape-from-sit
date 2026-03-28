#pragma once
#include "Engine/Core/Component.hpp"

namespace Engine
{
    class GameObject;
}

namespace Game
{

    class Inventory : public Engine::Component
    {
    public:
        static const int MAX_SLOTS = 3;
        Engine::GameObject *slots[MAX_SLOTS] = {nullptr, nullptr, nullptr};
        int activeSlot = 0;

        void Update() override;
        bool AddItem(Engine::GameObject *itemObj);
        void UseActiveItem();
    };

}