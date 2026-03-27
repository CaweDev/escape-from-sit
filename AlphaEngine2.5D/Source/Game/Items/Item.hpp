#pragma once
#include "Engine/Core/Component.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <cmath>

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

        void Initialize() override
        {
            struct ItemConfig : public Engine::IConfig
            {
                float floatHeight = 0.15f;
                float floatSpeed = 2.0f;

                void FromJson(const Json::Value& j) override
                {
                    floatHeight = j.get("floatHeight", floatHeight).asFloat();
                    floatSpeed = j.get("floatSpeed", floatSpeed).asFloat();
                }
            };

            static ItemConfig cfg;
            static bool loaded = false;
            if (!loaded)
            {
                Engine::ConfigManager::ReadConfig("Assets/Config/Game/item.json", cfg);
                if (cfg.floatHeight < 0.0f) cfg.floatHeight = 0.0f;
                if (cfg.floatSpeed < 0.0f) cfg.floatSpeed = 0.0f;
                loaded = true;
            }

            floatHeight = cfg.floatHeight;
            floatSpeed = cfg.floatSpeed;
            if (transform)
                baseZ = transform->position.z;
        }

        void Update() override
        {
            if (isCollected || !transform) return;

            floatTime += Engine::Time::deltaTime * floatSpeed;
            transform->position.z = baseZ + std::sin(floatTime) * floatHeight;
        }
    };
}