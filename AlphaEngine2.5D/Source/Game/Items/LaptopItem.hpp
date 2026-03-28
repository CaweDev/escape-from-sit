#pragma once
#include "Item.hpp"
#include <vector>

namespace Engine
{
    class Rigidbody2D;
}

namespace Game
{

    class LaptopItem : public Item
    {
    public:
        void Initialize() override;
        void Update() override;
        void OnDestroy() override;
        void Use(Engine::GameObject *user) override;
        float timer = 1.5f;

    private:
        bool isThrown = false;
        bool hasLanded = false;
        Engine::Rigidbody2D* rigidbody2D;
        std::vector<Engine::GameObject *> distractedEntities;
    };

}