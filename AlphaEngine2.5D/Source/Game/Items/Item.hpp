#pragma once
#include "Engine/Core/Component.hpp"

namespace Game
{
    class Item: public Engine::Component
    {
        public:
        bool isCollected = false;

        float floatTime = 0.0f;
        float baseZ = 0.0f;
        float floatHeight = 0.15f;
        float floatSpeed = 2.0f;
    };
}