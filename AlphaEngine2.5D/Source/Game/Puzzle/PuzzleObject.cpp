#include "PuzzleObject.h"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <cmath>

namespace
{
    struct PuzzleObjectConfig : public Engine::IConfig
    {
        float floatAmplitude = 0.02f;
        float floatSpeed = 2.0f;
        float rotateSpeed = 10.0f;

        void FromJson(const Json::Value& j) override
        {
            floatAmplitude = j.get("floatAmplitude", floatAmplitude).asFloat();
            floatSpeed = j.get("floatSpeed", floatSpeed).asFloat();
            rotateSpeed = j.get("rotateSpeed", rotateSpeed).asFloat();
        }
    };

    PuzzleObjectConfig g_puzzleObjectConfig;
    bool g_puzzleObjectConfigLoaded = false;

    const PuzzleObjectConfig& GetPuzzleObjectConfig()
    {
        if (!g_puzzleObjectConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/puzzle_object.json", g_puzzleObjectConfig);
            g_puzzleObjectConfigLoaded = true;
        }
        return g_puzzleObjectConfig;
    }
}

void PuzzleObject::Update()
{
    if (!gameObject || !gameObject->transform) return;

    const auto& cfg = GetPuzzleObjectConfig();
    const float t = Engine::Time::time;

    const float baseZ = (Reference.BookPosition.z != 0.0f) ? Reference.BookPosition.z : gameObject->transform->position.z;
    gameObject->transform->position.z = baseZ + std::sin(t * cfg.floatSpeed) * cfg.floatAmplitude;
    gameObject->transform->rotation += Engine::Time::deltaTime * cfg.rotateSpeed;
}
