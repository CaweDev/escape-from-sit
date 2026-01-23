
#pragma once

#include "Engine/Scene/Scene.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Graphics/TileMapRenderer.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Physics/CircleCollider2D.hpp"
#include "Engine/Physics/TileMapCollider2D.hpp"
#include "Engine/Core/Time.hpp"

#include "Game/FPSController.hpp"
#include "Game/Player.hpp"
#include "Game/SecurityGuard.hpp"

#include <Engine/Graphics/SpriteRenderer.hpp>

#include <iostream>
#include <vector>
#include <memory>
#include <cmath>
#include <string>

namespace Game {

    using namespace Engine;

    class FPSScene : public Scene {
    public:
        FPSScene() : Scene("FPSScene") {}

        bool gameEnded = false;

        FPSController* playerController = nullptr;
        Player* playerData = nullptr;
        Transform* playerTf = nullptr;

       
        Vector2 exitPos = Vector2(14.5f, 1.5f);
        float winRadius = 0.6f;
        bool printedWin = false;
        std::vector<GameObject*> powerUps;
        std::vector<float> powerUpBaseZ;
        float powerUpTime = 0.0f;
        void OnLoad() override {
            const int mapData[] = {
                1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
                1,0,0,0,1,0,1,1,1,1,0,1,1,1,0,1,
                1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,
                1,0,1,0,1,1,1,0,1,1,1,0,0,1,0,1,
                1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1,
                1,0,1,0,1,0,1,1,1,0,1,0,1,1,0,1,
                1,0,1,0,0,0,1,0,1,0,0,0,0,1,0,1,
                1,0,1,1,1,0,1,0,1,0,0,0,0,0,0,1,
                1,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,
                1,0,1,1,1,1,1,0,1,0,0,0,1,0,0,1,
                1,0,0,0,0,0,0,0,0,0,1,0,1,0,0,1,
                1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
            };

          
            auto world = CreateGameObject("World");
            auto map = world->AddComponent<TileMap>();
            map->LoadFromArray(mapData, 16, 13);

            auto renderer = world->AddComponent<TileMapRenderer>(); 
            renderer->wallColorNS = Color(0.8f, 0.2f, 0.2f, 1.0f); 
            renderer->wallColorEW = Color(0.6f, 0.15f, 0.15f, 1.0f);
            renderer->ceilingColor = Color(0.1f, 0.1f, 0.2f, 1.0f);
            renderer->floorColor = Color(0.3f, 0.25f, 0.2f, 1.0f); { 
             auto skySprite = std::make_shared<Sprite>();
             skySprite->Load("Assets/Tilemap/modern_school_ceiling.png");
             auto groundSprite = std::make_shared<Sprite>();
             groundSprite->Load("Assets/Tilemap/modern_school_floor.png"); 
             auto wallSprite = std::make_shared<Sprite>(); 
             wallSprite->Load("Assets/Tilemap/modern_school_wall.png"); 
             renderer->SetCeilingSprite(skySprite); 
             renderer->SetFloorSprite(groundSprite);
             renderer->SetWallSprite(wallSprite); }

            auto tileMapCollider = world->AddComponent<TileMapCollider2D>();
            tileMapCollider->minHeight = 0.0f;
            tileMapCollider->maxHeight = 3.0f;

          
            auto player = CreateGameObject("Player");
            player->AddComponent<Camera>();

            auto rb = player->AddComponent<Rigidbody2D>();
            rb->gravityScale = 1.0f;
            rb->drag = 5.0f;

            auto col = player->AddComponent<CircleCollider2D>();
            col->radius = 0.2f;
            col->minHeight = 0.0f;
            col->maxHeight = 1.8f;

            playerController = player->AddComponent<FPSController>();
            playerData = player->AddComponent<Player>();
            playerTf = player->transform;

            playerTf->position = Vector3(1.5f, 1.5f);
            playerTf->rotation = 0.0f;

            
            auto exit = CreateGameObject("Exit");
            exit->transform->position = exitPos;

            auto exitSR = exit->AddComponent<SpriteRenderer>();
            auto exitSprite = std::make_shared<Sprite>();
            exitSprite->Load("Assets/PlanetTexture.png", 500, 500);
            exitSR->SetSprite(exitSprite);
            exitSR->billboard = true;
            exitSR->sortingOrder = 95;

         
            auto enemy = CreateGameObject("SecurityGuard");
            enemy->transform->position = Vector2(3.5f, 1.5f);

            auto guard = enemy->AddComponent<SecurityGuard>();
            guard->tileMap = map;
            guard->playerTf = playerTf;
            guard->playerData = playerData;
            guard->patrolSpeed = 1.5f;
            guard->chaseSpeed = 2.4f;

            auto enemyRb = enemy->AddComponent<Rigidbody2D>();
            enemyRb->bodyType = RigidbodyType2D::Kinematic;

            auto enemyCol = enemy->AddComponent<CircleCollider2D>();
            enemyCol->radius = 0.5f;
            enemyCol->minHeight = 0.0f;
            enemyCol->maxHeight = 2.0f;

            auto enemySR = enemy->AddComponent<SpriteRenderer>();
            auto enemySprite = std::make_shared<Sprite>();
            enemySprite->Load("Assets/PlanetTexture.png", 500, 500);
            enemySR->SetSprite(enemySprite);
            enemySR->billboard = true;
            enemySR->sortingOrder = 100;

            
            auto powerup1 = std::make_shared<Sprite>();
            powerup1->Load("Assets/powerup1.png", 20,20);

            auto powerup2 = std::make_shared<Sprite>();
            powerup2->Load("Assets/powerup2.png", 20, 20);

            auto powerup3 = std::make_shared<Sprite>();
            powerup3->Load("Assets/powerup3.png", 20, 20);

            auto powerup4 = std::make_shared<Sprite>();
            powerup4->Load("Assets/powerup4.png", 20, 20);

            auto powerup5 = std::make_shared<Sprite>();
            powerup5->Load("Assets/powerup5.png", 20, 20);


            const float PU_Z = 0.20f;
            const int PU_ORDER = 300;

         
            auto pu1 = CreateGameObject("PU_Top_1");
            pu1->transform->position = Vector3(2.5f, 1.5f, PU_Z);
            pu1->transform->localScale = Vector2(0.25f, 0.25f);

            auto pu1SR = pu1->AddComponent<SpriteRenderer>();
            pu1SR->SetSprite(powerup1);
            pu1SR->billboard = true;
            pu1SR->sortingOrder = PU_ORDER;
            powerUps.push_back(pu1); 
            powerUpBaseZ.push_back(pu1->transform->position.z);

            auto pu2 = CreateGameObject("PU_Top_2");
            pu2->transform->position = Vector3(5.5f, 1.5f, PU_Z);
            pu2->transform->localScale = Vector2(0.25f, 0.25f);

            auto pu2SR = pu2->AddComponent<SpriteRenderer>();
            pu2SR->SetSprite(powerup2);
            pu2SR->billboard = true;
            pu2SR->sortingOrder = PU_ORDER;
            powerUps.push_back(pu2); 
            powerUpBaseZ.push_back(pu2->transform->position.z);


            auto pu3 = CreateGameObject("PU_Top_3");
            pu3->transform->position = Vector3(8.5f, 1.5f, PU_Z);
            pu3->transform->localScale = Vector2(0.25f, 0.25f);

            auto pu3SR = pu3->AddComponent<SpriteRenderer>();
            pu3SR->SetSprite(powerup3);
            pu3SR->billboard = true;
            pu3SR->sortingOrder = PU_ORDER;
            powerUps.push_back(pu3); 
            powerUpBaseZ.push_back(pu3->transform->position.z);

            auto pu4 = CreateGameObject("PU_Top_4");
            pu4->transform->position = Vector3(11.5f, 1.5f, PU_Z);
            pu4->transform->localScale = Vector2(0.25f, 0.25f);

            auto pu4SR = pu4->AddComponent<SpriteRenderer>();
            pu4SR->SetSprite(powerup4);
            pu4SR->billboard = true;
            pu4SR->sortingOrder = PU_ORDER;
            powerUps.push_back(pu4); 
            powerUpBaseZ.push_back(pu4->transform->position.z);

            auto pu5 = CreateGameObject("PU_Top_5");
            pu5->transform->position = Vector3(14.5f, 1.5f, PU_Z);
            pu5->transform->localScale = Vector2(0.25f, 0.25f);

            auto pu5SR = pu5->AddComponent<SpriteRenderer>();
            pu5SR->SetSprite(powerup5);
            pu5SR->billboard = true;
            pu5SR->sortingOrder = PU_ORDER;
            powerUps.push_back(pu5); 
            powerUpBaseZ.push_back(pu5->transform->position.z);
           
            auto pu6 = CreateGameObject("PU_Left_1");
            pu6->transform->position = Vector3(1.5f, 3.5f, PU_Z);
            pu6->transform->localScale = Vector2(0.25f, 0.25f);

            auto pu6SR = pu6->AddComponent<SpriteRenderer>();
            pu6SR->SetSprite(powerup5);
            pu6SR->billboard = true;
            pu6SR->sortingOrder = PU_ORDER;

            powerUps.push_back(pu6); 
            powerUpBaseZ.push_back(pu6->transform->position.z);



            auto cctvSprite = std::make_shared<Sprite>();
            cctvSprite->Load("Assets/cctv.png",64, 64);
            const float CCTV_Z = 0.85f;
            const float EPS = 0.02f;

            struct CCTVInfo { Vector3 pos; };
            CCTVInfo cams[6] = {
               
                { Vector3(1.0f + EPS,  2.4f, CCTV_Z) },
                { Vector3(1.0f + EPS,  9.4f, CCTV_Z) },

            
                { Vector3(15.0f - EPS, 2.4f, CCTV_Z) },
                { Vector3(15.0f - EPS, 9.4f, CCTV_Z) },

              
                { Vector3(5.5f,  1.0f + EPS, CCTV_Z) },
                { Vector3(10.5f, 1.0f + EPS, CCTV_Z) }
            };

            for (int i = 0; i < 6; ++i) {
                auto cam = CreateGameObject("CCTV_" + std::to_string(i + 1));
                cam->transform->position = cams[i].pos;
       
                cam->transform->localScale = Vector2(0.25f, 0.25f);
                auto sr = cam->AddComponent<SpriteRenderer>();
                sr->SetSprite(cctvSprite);

                // Must be true in your raycaster to render consistently
                sr->billboard = true;

                // Draw on top
                sr->sortingOrder = 300;
            }


        }

            

        void OnUpdate() override {
            powerUpTime += Engine::Time::deltaTime;
            
            if (!playerData || !playerTf) return;
            if (playerData->GetState() != PlayerState::Normal) return;

            const float floatAmplitude = 0.15f;
            const float floatSpeed = 2.0f;
            for (size_t i = 0; i < powerUps.size(); ++i) {
                auto& pos = powerUps[i]->transform->position;
                pos.z = powerUpBaseZ[i]+ std::sin(powerUpTime * floatSpeed + (float)i) * floatAmplitude;
            }


            Vector2 p(playerTf->position.x, playerTf->position.y);
            float dx = exitPos.x - p.x;
            float dy = exitPos.y - p.y;

            if (!printedWin && (dx * dx + dy * dy) <= winRadius * winRadius) {
                printedWin = true;
                gameEnded = true;
                playerData->Win();
                std::cout << "YOU ESCAPED. WIN!\n";
            }

        }
    };

} // namespace Game
