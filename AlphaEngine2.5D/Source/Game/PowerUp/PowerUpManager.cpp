#include "PowerUpManager.h"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Physics/CircleCollider2D.hpp"
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

        auto collider = powerUpObj->AddComponent<Engine::CircleCollider2D>();
        collider->radius = 0.5f;
        collider->minHeight = 0.0f;
        collider->maxHeight = 2.0f;

        auto sr = powerUpObj->AddComponent<Engine::SpriteRenderer>();

        // Choose correct placeholder sprite path based on type
        switch (data.type) {
        case PowerUpType::SpeedBoost:      sr->SetSprite(Engine::AssetManager::GetSprite("Assets/powerup1.png")); break;
        case PowerUpType::InfiniteStamina: sr->SetSprite(Engine::AssetManager::GetSprite("Assets/powerup2.png")); break;
        case PowerUpType::Shield:          sr->SetSprite(Engine::AssetManager::GetSprite("Assets/powerup3.png")); break;
        case PowerUpType::Invisibility:    sr->SetSprite(Engine::AssetManager::GetSprite("Assets/powerup4.png")); break;
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

        // Testing: Spawn Room (Player starts at 2.5, 2.5)
        allPowerUps.push_back({ 0, {5.5f, 3.5f, 0.0f}, PowerUpType::SpeedBoost,      "Speed Boost",      10.0f });
        allPowerUps.push_back({ 1, {6.5f, 3.5f, 0.0f}, PowerUpType::InfiniteStamina, "Infinite Stamina", 15.0f });
        allPowerUps.push_back({ 2, {7.5f, 3.5f, 0.0f}, PowerUpType::Shield,          "Shield",           20.0f });
        allPowerUps.push_back({ 3, {8.5f, 3.5f, 0.0f}, PowerUpType::Invisibility,    "Invisibility",     8.0f });

        // Sector 1: Example Coordinates
        allPowerUps.push_back({ 4, {8.5f, 12.5f, 0.0f},  PowerUpType::InfiniteStamina, "Infinite Stamina", 15.0f });
        allPowerUps.push_back({ 5, {12.5f, 18.5f, 0.0f}, PowerUpType::Invisibility,    "Invisibility",     8.0f });

        // Sector 2: Example Coordinates
        allPowerUps.push_back({ 6, {35.5f, 22.5f, 0.1f}, PowerUpType::Shield,          "Shield",           20.0f });
        allPowerUps.push_back({ 7, {40.5f, 28.5f, 0.1f}, PowerUpType::SpeedBoost,      "Speed Boost",      10.0f });
        allPowerUps.push_back({ 8, {45.5f, 35.5f, 0.1f}, PowerUpType::Invisibility,    "Invisibility",     8.0f });

        // Sector 3: Example Coordinates
        allPowerUps.push_back({ 9, {65.5f, 40.5f, 0.2f}, PowerUpType::InfiniteStamina, "Infinite Stamina", 15.0f });
        allPowerUps.push_back({ 10, {75.5f, 48.5f, 0.2f}, PowerUpType::SpeedBoost,      "Speed Boost",      10.0f });
        allPowerUps.push_back({ 11, {85.5f, 55.5f, 0.2f}, PowerUpType::Shield,          "Shield",           20.0f });
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