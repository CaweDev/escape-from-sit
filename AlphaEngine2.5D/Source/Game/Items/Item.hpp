#pragma once
#include <string>
#include "Engine/Core/Component.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <cmath>

#include "Engine/Interaction/Interactable.hpp"

namespace Game
{

    class Item : public Engine::Component
    {
    public:
        std::string itemName = "Unknown";
        void Initialize() override;

        virtual void Use(Engine::GameObject *user) = 0;

    private:
        Engine::Interactable *interactable = nullptr;
        Item *itemData = nullptr;

        void OnCollect();
    };

}