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
#include "Game/CCTVCamera.hpp"
#include "Game/UI/GameScene/PlayerHUD.hpp"

#include <Engine/Graphics/SpriteRenderer.hpp>
#include "Engine/Input/KeyCode.hpp"

#include <iostream>
#include <vector>
#include <memory>
#include <cmath>
#include <string>

#include "Game/Puzzle/PuzzleManager.h"

namespace Game {

    using namespace Engine;

    class FPSScene : public Scene {
    public:
        FPSScene() : Scene("FPSScene") {}

        bool gameEnded = false;

        FPSController* playerController = nullptr;
        Player* playerData = nullptr;
        Transform* playerTf = nullptr;
        Camera* playerCam = nullptr;
        SecurityGuard* guardComp = nullptr;
        Vector3 playerViewPos{};
        float playerViewRot = 0.0f;

        std::vector<Vector3> cctvViewPos;
        std::vector<float>   cctvViewRot;
        int activeCCTV = -1;

        std::vector<CCTVCamera*> cctvSensors;

        Vector2 exitPos = Vector2(14.5f, 1.5f);
        float winRadius = 0.6f;
        bool printedWin = false;

        std::vector<GameObject*> powerUps;
        std::vector<float> powerUpBaseZ;
        float powerUpTime = 0.0f;

        void SwitchToPlayerCam() {
            activeCCTV = -1;
            if (!playerTf) return;

            playerTf->position = playerViewPos;
            playerTf->rotation = playerViewRot;

            std::cout << "\n[VIEW] Player Camera\n";
        }

        void SwitchToCCTV(int idx) {
            if (!playerTf) return;
            if (idx < 0 || idx >= (int)cctvViewPos.size()) return;

            activeCCTV = idx;

            playerTf->position = cctvViewPos[idx];
            playerTf->rotation = cctvViewRot[idx];

            std::cout << "\n[VIEW] CCTV " << (idx + 1) << "\n";
        }

        
        void ToggleCCTV(int idx) {
            if (activeCCTV == idx) SwitchToPlayerCam();
            else SwitchToCCTV(idx);
        }

        void OnLoad() override {
            const int mapWidth = 124;
            const int mapHeight = 43;

            static const char* mapRows[mapHeight] = {
            "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
            "1000010000010000100000010000100000001000000001000000010000001000000100000000010000000000000000000000000000000000000000000001",
            "1000010000010000100000010000100000001000000001000000010000001000000100000000010000000000000000000000000011111000111110000001",
            "1001111001110011100000111101100011111111001111101111111011111110011111101111110000000000000010000000000010000000000000000001",
            "1001000000000000000000000000000000000000000000000000000000000000000000000000010000000000000010000000000010000000000000000001",
            "1001000000000000000000000000000000000000000000000000000000000000000000000000011111100011111110000000000001111111000111111111",
            "1001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000100100000000000000000010001",
            "1001000000011111110011111111111100111111111111110011111111111110011111100000000000000000000000000100100000000000000000010001",
            "1000000000010000000000001000000000000000100000000000000010000000000000111111111111000111111111111100111111111100000100010001",
            "1001000000010000000000001000000000000000100000000000000010000000000000100000000000000000000010000000000010000100000100010001",
            "1111000000011111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111110000100000100000001",
            "1111000000011111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111110000100000100010001",
            "1111000000011111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111110000000000100010001",
            "1111000000011110000000000010000000000001000000000010000000000000000000010000000000000000000000000000011110000100000100010001",
            "1111000000011110000000000010000000000001000000000010000000000000000000010000000000000000000000000000011110000100000100010001",
            "1111000000011110000000000011100000000111111110011111111111111100000011111111111110001111111111100000011110000100000100000001",
            "1111000000011110000000000000000000000000000000000000000000000000000000000000000000000000000000100000011110000100000100010001",
            "1111000000011110000000000000000000000000000000000000000000000000000000000000000000000000000000000000011110000100000100010001",
            "1111000000011110000000000010000000000000000000000000000000000000000000000000000000000000000000100000011110000100000000011011",
            "1111000000011111111111111110000001111111111111111111111111111111111111111111111111111111000000111111111110000100000100010001",
            "1111000000011111111111111110000001111111111111111111111111111111111111111111111111111111000000111111111110000100000100000001",
            "1111000000011111111111111110000001111111111111111111111111111111111111111111111111111111000000111111111110000100000110110001",
            "1111000000011111111111111110000001111000000010000000000000000000000001000000001111111111000000111111111110000100000100010001",
            "1111000000011111111111111110000001111000000000000000000000000000000000000000001111111111000000111111111110000000000100000001",
            "1111000000011111111111111110000001111000000010000000000000000000000001000000001111111111000000111111111110000100000100010001",
            "1111000000011111111111111110000001111111111111111111111111111111100001111111111111111111000000111111111110000100000100010001",
            "1111000000000111111111111110000001111111111111111111111111111111100001111111111111111111000000111111111111111100000111111011",
            "1000000000000011111111111110000001111111111111111111111111111111100001111111111110000001000000100000000010000000000000000001",
            "1000000000000011111111111110000001111111111111111111111111111111100001111111111110000001000000100000000000000000000000000001",
            "1000000000000011111111111110000001111111111111111111111111111111100001111111111110000001000000000000000010000000000000000001",
            "1000000000000010000000000010000001000001000000000000001111111111100001111111111110000001000000100000000010000000000000000001",
            "1000000000000010000000000010000001000001000000000000000000000000000001111111111110000001000000111110011110000000000000000001",
            "1000000000000011111100111110000001110011000000000000000000000000000001111111111110000000000000000000000000000000000000000001",
            "1000000000000000000000000000000000000000000000000000011111111111100001111111111110000000000000000000000000000000000000000001",
            "1000000000000000000000000000000000000000000000000111111111111111100001111111111110000001000000000000000000000000000000000001",
            "1000000000000011111111111000111000000000000000000100011111111111100001111111111110000001000000000000000000000000000000000001",
            "1000000000000010000000000000001000000000000000000000011111111111100001111111111111111111000000000000000000000000000000000001",
            "1000000000000010000000000000001000000000000000000100011111111111100000000000000000000000000000000000000000000000000000000001",
            "1111111111001111111111111111111111111111111111111111111111111111100000000000000000000000000000000000000000000000000000000001",
            "1000000000000011111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
            "1000000000000011111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
            "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
            "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111"
            };

            int worldMap[mapWidth * mapHeight];

            for (int y = 0; y < mapHeight; ++y) {
                for (int x = 0; x < mapWidth; ++x) {
                    worldMap[y * mapWidth + x] = mapRows[y][x] - '0'; // '0'->0, '1'->1
                }
            }

            auto world = CreateGameObject("World");
            auto map = world->AddComponent<TileMap>();
            map->LoadFromArray(worldMap, mapWidth, mapHeight);  // <-- 124, 43

            auto renderer = world->AddComponent<TileMapRenderer>();
            renderer->wallColorNS = Color(0.8f, 0.2f, 0.2f, 1.0f);
            renderer->wallColorEW = Color(0.6f, 0.15f, 0.15f, 1.0f);
            renderer->ceilingColor = Color(0.1f, 0.1f, 0.2f, 1.0f);
            renderer->floorColor = Color(0.3f, 0.25f, 0.2f, 1.0f);

            {
                auto skySprite = std::make_shared<Sprite>();
                skySprite->Load("Assets/Tilemap/modern_school_ceiling.png");
                auto groundSprite = std::make_shared<Sprite>();
                groundSprite->Load("Assets/Tilemap/modern_school_floor.png");
                auto wallSprite = std::make_shared<Sprite>();
                wallSprite->Load("Assets/Tilemap/modern_school_wall.png");
                renderer->SetCeilingSprite(skySprite);
                renderer->SetFloorSprite(groundSprite);
                renderer->SetWallSprite(wallSprite);
            }

            auto tileMapCollider = world->AddComponent<TileMapCollider2D>();
            tileMapCollider->minHeight = 0.0f;
            tileMapCollider->maxHeight = 3.0f;

            
            auto player = CreateGameObject("Player");
            playerCam = player->AddComponent<Camera>();

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

            // Add PlayerHUD to display minimap and full map
            auto playerHUD = player->AddComponent<PlayerHUD>();
            playerHUD->SetTileMap(map);
            playerHUD->SetPlayerTransform(playerTf);

            playerTf->position = Vector3(2.5f, 2.5f, 0.0f);
            playerTf->rotation = 0.0f;

            playerViewPos = playerTf->position;
            playerViewRot = playerTf->rotation;

            
            auto exit = CreateGameObject("Exit");
            exit->transform->position = exitPos;

            auto exitSR = exit->AddComponent<SpriteRenderer>();
            auto exitSprite = std::make_shared<Sprite>();
            exitSprite->Load("Assets/PlanetTexture.png");
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
            guardComp = guard;


            auto enemyRb = enemy->AddComponent<Rigidbody2D>();
            enemyRb->bodyType = RigidbodyType2D::Kinematic;

            auto enemyCol = enemy->AddComponent<CircleCollider2D>();
            enemyCol->radius = 0.5f;
            enemyCol->minHeight = 0.0f;
            enemyCol->maxHeight = 2.0f;

            auto enemySR = enemy->AddComponent<SpriteRenderer>();
            auto enemySprite = std::make_shared<Sprite>();
            enemySprite->Load("Assets/PlanetTexture.png");
            enemySR->SetSprite(enemySprite);
            enemySR->billboard = true;
            enemySR->sortingOrder = 100;

            
            auto powerup1 = std::make_shared<Sprite>(); powerup1->Load("Assets/powerup1.png");
            auto powerup2 = std::make_shared<Sprite>(); powerup2->Load("Assets/powerup2.png");
            auto powerup3 = std::make_shared<Sprite>(); powerup3->Load("Assets/powerup3.png");
            auto powerup4 = std::make_shared<Sprite>(); powerup4->Load("Assets/powerup4.png");
            auto powerup5 = std::make_shared<Sprite>(); powerup5->Load("Assets/powerup5.png");

            const float PU_Z = 0.20f;
            const int PU_ORDER = 300;

            auto SpawnPU = [&](char const* name, float x, float y, std::shared_ptr<Sprite> spr) {
                auto pu = CreateGameObject(name);
                pu->transform->position = Vector3(x, y, PU_Z);
                pu->transform->localScale = Vector2(0.25f, 0.25f);

                auto sr = pu->AddComponent<SpriteRenderer>();
                sr->SetSprite(spr);
                sr->billboard = true;
                sr->sortingOrder = PU_ORDER;

                powerUps.push_back(pu);
                powerUpBaseZ.push_back(pu->transform->position.z);
                };

            SpawnPU("PU1", 2.5f, 1.5f, powerup1);
            SpawnPU("PU2", 5.5f, 1.5f, powerup2);
            SpawnPU("PU3", 8.5f, 1.5f, powerup3);
            SpawnPU("PU4", 11.5f, 1.5f, powerup4);
            SpawnPU("PU5", 14.5f, 1.5f, powerup5);
            SpawnPU("PU6", 1.5f, 3.5f, powerup5);

            
            auto cctvSprite = std::make_shared<Sprite>();
            cctvSprite->Load("Assets/cctv.png");

            const float CCTV_Z = 0.85f;
            const float EPS = 0.02f;

            struct CCTVInfo { Vector3 pos; float rot; };
            CCTVInfo cams[6] = {
                { Vector3(1.0f + EPS,  2.4f, CCTV_Z),   0.0f   },
                { Vector3(1.0f + EPS,  9.4f, CCTV_Z),   0.0f   },
                { Vector3(15.0f - EPS, 2.4f, CCTV_Z), 180.0f   },
                { Vector3(15.0f - EPS, 9.4f, CCTV_Z), 180.0f   },
                { Vector3(5.5f,  1.0f + EPS, CCTV_Z), 270.0f   },
                { Vector3(10.5f, 1.0f + EPS, CCTV_Z), 270.0f   }
            };

            for (int i = 0; i < 6; ++i) {
                
                Vector3 view = cams[i].pos;
                view.z = 1.6f; 
                cctvViewPos.push_back(view);
                cctvViewRot.push_back(cams[i].rot);

                auto camObj = CreateGameObject("CCTV_" + std::to_string(i + 1));
                camObj->transform->position = cams[i].pos;
                camObj->transform->rotation = cams[i].rot;
                camObj->transform->localScale = Vector2(0.25f, 0.25f);

                auto sr = camObj->AddComponent<SpriteRenderer>();
                sr->SetSprite(cctvSprite);
                sr->billboard = true;
                sr->sortingOrder = 300;

                auto sensor = camObj->AddComponent<CCTVCamera>();
                sensor->id = i + 1;
                sensor->tile = map;
                sensor->tf = playerTf;
                sensor->detectRadius = 3.0f;
                sensor->maxRange = 6.0f;
                sensor->coneWidth = 70.0f;
                sensor->needLOS = true;
                cctvSensors.push_back(sensor);
            }

            SwitchToPlayerCam();
            PuzzleManager::BookDataInit();
            PuzzleManager::Init();
        }

        void OnUpdate() override {
         
            std::cout << '\n' << Camera::main->transform->position.x << "," << Camera::main->transform->position.y << "  \n";
            powerUpTime += Engine::Time::deltaTime;
            const float floatAmplitude = 0.15f;
            const float floatSpeed = 2.0f;

            for (size_t i = 0; i < powerUps.size(); ++i) {
                auto& pos = powerUps[i]->transform->position;
                pos.z = powerUpBaseZ[i] + std::sin(powerUpTime * floatSpeed + (float)i) * floatAmplitude;
            }

      
            if (Input::GetKeyDown(KeyCode::Alpha1)) ToggleCCTV(0);
            if (Input::GetKeyDown(KeyCode::Alpha2)) ToggleCCTV(1);
            if (Input::GetKeyDown(KeyCode::Alpha3)) ToggleCCTV(2);
            if (Input::GetKeyDown(KeyCode::Alpha4)) ToggleCCTV(3);
            if (Input::GetKeyDown(KeyCode::Alpha5)) ToggleCCTV(4);
            if (Input::GetKeyDown(KeyCode::Alpha6)) ToggleCCTV(5);
            if (Input::GetKeyDown(KeyCode::Alpha0)) SwitchToPlayerCam();

         

            for (auto* s : cctvSensors) {
                if (s && s->hasLastSeen) {
                    std::cout << "\r[CCTV " << s->id << "] LastSeenTile=("
                        << (int)s->lastSeenTile.x << "," << (int)s->lastSeenTile.y << ")   "
                        << std::flush;
                    break;
                }
            }



            if (!playerData || !playerTf) return;
            if (playerData->GetState() != PlayerState::Normal) return;

      
            Vector2 p(playerTf->position.x, playerTf->position.y);
            float dx = exitPos.x - p.x;
            float dy = exitPos.y - p.y;

            if(guardComp) {
                for (auto* s : cctvSensors) {
                    if (s && s->hasLastSeen) {
                        guardComp->OnCCTVSpotted(s->lastSeenWorld);

                        // consume so it triggers once per detection
                        s->hasLastSeen = false;
                        break;
                    }
                }
            }

            if (!printedWin && (dx * dx + dy * dy) <= winRadius * winRadius) {
                printedWin = true;
                gameEnded = true;
                playerData->Win();
                std::cout << "YOU ESCAPED. WIN!\n";
            }
        }
    };

} // namespace Game
