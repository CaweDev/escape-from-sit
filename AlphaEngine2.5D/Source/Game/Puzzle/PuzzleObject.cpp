/*!
 * \file AlphaEngine2.5D\Source\Game\Puzzle\PuzzleObject.cpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief [AUTO] Added Doxygen file header
 */

#include "PuzzleObject.hpp"
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

void Game::PuzzleObject::Initialize()
{
    if (gameObject && gameObject->transform) {
        baseZ = gameObject->transform->position.z;
    }
}

void Game::PuzzleObject::Update()
{
    if (!gameObject || !gameObject->transform) return;

    const auto& cfg = GetPuzzleObjectConfig();
    const float t = Engine::Time::time;

    gameObject->transform->position.z = baseZ + std::sin(t * cfg.floatSpeed) * cfg.floatAmplitude;
    gameObject->transform->rotation += Engine::Time::deltaTime * cfg.rotateSpeed;
}
