#include "PowerUpManager.h"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Interaction/Interactable.hpp"
#include "Engine/Core/AssetManager.hpp"

namespace Game {

    std::vector<PowerUpData> PowerUpManager::allPowerUps = {};

    void PowerUpManager::CreatePowerUpObject(PowerUpData data) {
        auto scene = Engine::SceneManager::GetActiveScene();
        if (!scene) return;

        auto powerUpObj = scene->CreateGameObject("PowerUp_" + data.name);
        powerUpObj->transform->position = data.position;
        powerUpObj->transform->rotation = 0.0f;
        powerUpObj->transform->localScale = Engine::Vector2(0.6f, 0.6f);

        auto rb = powerUpObj->AddComponent<Engine::Rigidbody2D>();
        rb->bodyType = Engine::RigidbodyType2D::Static;

        auto sr = powerUpObj->AddComponent<Engine::SpriteRenderer>();

        // Choose correct placeholder sprite path based on type
        switch (data.type) {
        case PowerUpType::SpeedBoost:      sr->SetSprite(Engine::AssetManager::GetSprite("Assets/Speed.png")); break;
        case PowerUpType::InfiniteStamina: sr->SetSprite(Engine::AssetManager::GetSprite("Assets/InfStamina.png")); break;
        case PowerUpType::Shield:          sr->SetSprite(Engine::AssetManager::GetSprite("Assets/Shield.png")); break;
        case PowerUpType::Invisibility:    sr->SetSprite(Engine::AssetManager::GetSprite("Assets/Invis.png")); break;
        }

        sr->billboard = true;
        sr->enabled = true;
        sr->sortingOrder = 100;

        // Must add interactable before the Component so Awake gets it cleanly
        powerUpObj->AddComponent<Engine::Interactable>();

        auto logicComp = powerUpObj->AddComponent<PowerUpObject>();
        logicComp->reference = data;
    }

    void PowerUpManager::PowerUpDataInit() {
        allPowerUps.clear();

        // Hallways only (hardcoded corridor coordinates)
        allPowerUps.push_back({ 4,  {18.5f,  8.5f, 0.1f}, PowerUpType::InfiniteStamina, "Infinite Stamina", 15.0f });
        allPowerUps.push_back({ 5,  {27.5f, 10.5f, 0.1f}, PowerUpType::Invisibility,    "Invisibility",      8.0f });

        allPowerUps.push_back({ 6,  {41.5f, 19.5f, 0.1f}, PowerUpType::Shield,          "Shield",           20.0f });
        allPowerUps.push_back({ 7,  {58.5f, 24.5f, 0.1f}, PowerUpType::SpeedBoost,      "Speed Boost",      10.0f });
        allPowerUps.push_back({ 8,  {72.5f, 27.5f, 0.1f}, PowerUpType::Invisibility,    "Invisibility",      8.0f });

        allPowerUps.push_back({ 9,  {86.5f, 31.5f, 0.1f}, PowerUpType::InfiniteStamina, "Infinite Stamina", 15.0f });
        allPowerUps.push_back({ 10, {103.5f, 36.5f, 0.1f}, PowerUpType::SpeedBoost,     "Speed Boost",      10.0f });
        allPowerUps.push_back({ 11, {116.5f, 39.5f, 0.1f}, PowerUpType::Shield,         "Shield",           20.0f });

        allPowerUps.push_back({ 12, {33.5f, 13.5f, 0.1f}, PowerUpType::SpeedBoost,      "Speed Boost",      10.0f });
        allPowerUps.push_back({ 13, {94.5f, 34.5f, 0.1f}, PowerUpType::SpeedBoost,      "Speed Boost",      10.0f });
        allPowerUps.push_back({ 14, {110.5f, 35.5f, 0.1f}, PowerUpType::Invisibility,   "Invisibility",      8.0f });
    }

    void PowerUpManager::Init() {
        PowerUpDataInit();
        for (const auto& data : allPowerUps) {
            CreatePowerUpObject(data);
        }
    }

    void PowerUpManager::FreeManager() {
        allPowerUps.clear();
    }

}