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

#include <Engine/Graphics/SpriteRenderer.hpp>
#include "Engine/Input/KeyCode.hpp"

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

            playerTf->position = Vector3(1.5f, 1.5f, 0.0f);
            playerTf->rotation = 0.0f;

            playerViewPos = playerTf->position;
            playerViewRot = playerTf->rotation;

            
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
            guardComp = guard;


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

            
            auto powerup1 = std::make_shared<Sprite>(); powerup1->Load("Assets/powerup1.png", 20, 20);
            auto powerup2 = std::make_shared<Sprite>(); powerup2->Load("Assets/powerup2.png", 20, 20);
            auto powerup3 = std::make_shared<Sprite>(); powerup3->Load("Assets/powerup3.png", 20, 20);
            auto powerup4 = std::make_shared<Sprite>(); powerup4->Load("Assets/powerup4.png", 20, 20);
            auto powerup5 = std::make_shared<Sprite>(); powerup5->Load("Assets/powerup5.png", 20, 20);

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
            cctvSprite->Load("Assets/cctv.png", 64, 64);

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
        }

        void OnUpdate() override {
         
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
