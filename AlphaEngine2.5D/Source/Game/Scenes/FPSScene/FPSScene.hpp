/**
 * @file FPSScene.hpp
 * @brief Demo scene for Doom-style raycasting with physics
 */
#pragma once

#include "Engine/Scene/Scene.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Graphics/TileMapRenderer.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Physics/CircleCollider2D.hpp"
#include "Engine/Physics/TileMapCollider2D.hpp"
#include "Game/FPSController.hpp"
#include <Engine/Graphics/SpriteRenderer.hpp>
#include "Game/Player.hpp"
#include "Game/SecurityGuard.hpp"

namespace Game {

using namespace Engine;

/**
 * @class FPSScene
 * @brief Doom-style first-person demo with physics
 */
class FPSScene : public Scene {
public:
    FPSScene() : Scene("FPSScene") {}

    FPSController* playerController = nullptr;

    void OnLoad() override {
        // Define a simple maze level
        const int mapData[] = {
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
            1,0,0,0,1,0,1,1,1,1,0,1,1,1,0,1,
            1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,
            1,0,1,0,1,1,1,0,1,1,1,0,0,1,0,1,
            1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1,
            1,0,1,0,1,0,1,1,1,0,1,0,1,1,0,1,
            1,0,1,0,0,0,1,0,1,0,0,0,0,1,0,1,
            1,0,1,1,1,0,1,0,1,0,1,1,0,1,0,1,
            1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
            1,0,1,1,1,1,1,0,1,1,1,1,1,1,0,1,
            1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        };

        // Create world object with TileMap
        auto world = CreateGameObject("World");
        auto map = world->AddComponent<TileMap>();
        map->LoadFromArray(mapData, 16, 13);

        auto renderer = world->AddComponent<TileMapRenderer>();
        renderer->wallColorNS = Color(0.8f, 0.2f, 0.2f, 1.0f);
        renderer->wallColorEW = Color(0.6f, 0.15f, 0.15f, 1.0f);
        renderer->ceilingColor = Color(0.1f, 0.1f, 0.2f, 1.0f);
        renderer->floorColor = Color(0.3f, 0.25f, 0.2f, 1.0f);
		{   // Load map sprites into TileMapRenderer
            auto skySprite = std::make_shared<Sprite>();
            skySprite->Load("Assets/Tilemap/modern_school_ceiling.png");
            auto groundSprite = std::make_shared<Sprite>();
            groundSprite->Load("Assets/Tilemap/modern_school_floor.png");
            auto wallSprite = std::make_shared<Sprite>();
            wallSprite->Load("Assets/Tilemap/modern_school_wall.png");
            // Set on TileMapRenderer
            renderer->SetCeilingSprite(skySprite);
            renderer->SetFloorSprite(groundSprite);
            renderer->SetWallSprite(wallSprite);
        }
        
        // Add tilemap collider for physics
        auto tileMapCollider = world->AddComponent<TileMapCollider2D>();
        tileMapCollider->minHeight = 0.0f;
        tileMapCollider->maxHeight = 3.0f; // Walls are 3 units tall
        
        // Create player with physics
        auto player = CreateGameObject("Player");
        player->AddComponent<Camera>();
        
        // Add physics components
        auto playerRb = player->AddComponent<Rigidbody2D>();
        playerRb->gravityScale = 1.0f;
        playerRb->drag = 5.0f; // Stop quickly when no input
        
        auto playerCollider = player->AddComponent<CircleCollider2D>();
        playerCollider->radius = 0.2f;
        playerCollider->minHeight = 0.0f;
        playerCollider->maxHeight = 1.8f;
        
        // Add controller (requires Rigidbody2D)
        playerController = player->AddComponent<FPSController>();
        player->AddComponent<Player>();
        player->transform->position = Vector3(1.5f, 1.5f);
        player->transform->rotation = 0.0f;

        // Create dummy enemy with collider
        auto enemy = CreateGameObject("Enemy");
        enemy->transform->position = Vector2(3.5f, 1.5f);
        enemy->transform->rotation = 0.0f;
        enemy->transform->localScale = Vector2(1.0f, 1.0f);

        auto guard = enemy->AddComponent<SecurityGuard>();
        //hardcoded point to point
       /* guard->pointA = Vector2(3.5f, 1.5f);
        guard->pointB = Vector2(6.5f, 1.5f);*/

        //add tilemap pointer to guard
        guard->tileMap = map;

        guard->speed = 1.5f;
        guard->arriveDistance = 0.1f;

        // Enemy collider (static body)
        auto enemyRb = enemy->AddComponent<Rigidbody2D>();
        enemyRb->bodyType = RigidbodyType2D::Kinematic;
        
        auto enemyCollider = enemy->AddComponent<CircleCollider2D>();
        enemyCollider->radius = 0.5f;
        enemyCollider->minHeight = 0.0f;
        enemyCollider->maxHeight = 2.0f;
        
        // Enemy sprite
        auto enemySR = enemy->AddComponent<SpriteRenderer>();
        std::shared_ptr<Sprite> enemySprite = std::make_shared<Sprite>();
        enemySprite->Load("../Assets/PlanetTexture.png", 500, 500);
        enemySR->SetSprite(enemySprite);
        enemySR->billboard = true;
        enemySR->enabled = true;
        enemySR->sortingOrder = 100;
    }

    
    void OnUpdate() override {
        // Physics and rendering handled by components
    }
};

} // namespace Game
