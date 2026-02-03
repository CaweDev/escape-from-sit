#pragma once

#include "Engine/Scene/Scene.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Graphics/TileMapRenderer.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Physics/CircleCollider2D.hpp"
#include "Engine/Physics/TileMapCollider2D.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/AssetManager.hpp"

#include "Game/FPSController.hpp"
#include "Game/Player.hpp"
#include "Game/SecurityGuard.hpp"
#include "Game/CCTVCamera.hpp"
#include "Game/UI/GameScene/PlayerHUD.hpp"
#include "Game/Puzzle/PuzzleManager.h"
#include "Game/CCTVPlacement.hpp"

#include <Engine/Graphics/SpriteRenderer.hpp>
#include "Engine/Input/KeyCode.hpp"

#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <fstream>

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
        struct CCTVInfo { Vector3 pos; float rotDeg; };

  

        
      
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

            std::vector<std::string> mapRows;
            {
                std::ifstream file("Assets/map.txt");
                if (!file.is_open()) {
                    std::cout << "ERROR: Failed to open map.txt (check working directory)\n";
                    return;
                }

                std::string line;
                while (std::getline(file, line)) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    if (!line.empty()) mapRows.push_back(line);
                }
                file.close();

                if (mapRows.empty()) {
                    std::cout << "ERROR: map.txt is empty\n";
                    return;
                }
            }

            const int mapHeight = (int)mapRows.size();
            const int mapWidth = (int)mapRows[0].size();

            for (int y = 0; y < mapHeight; ++y) {
                if ((int)mapRows[y].size() != mapWidth) {
                    std::cout << "ERROR: map.txt row " << y << " has different width.\n";
                    return;
                }
            }

            std::cout << "Loaded map.txt: " << mapWidth << " x " << mapHeight << "\n";

            std::vector<int> worldMap(mapWidth * mapHeight, 0);
            for (int y = 0; y < mapHeight; ++y) {
                for (int x = 0; x < mapWidth; ++x) {
                    char c = mapRows[y][x];
                    int tileValue = 0;

                    if (c == '0') tileValue = 0;       // Empty
                    else if (c == '1') tileValue = 1;  // Wall
                    else if (c == 'D') tileValue = 10; // Door1
                    else if (c == 'E') tileValue = 11; // Door2
                    else if (c == 'F') tileValue = 12; // Door3
                    else if (c == 'G') tileValue = 13; // Door4
                    else tileValue = c - '0';          // Fallback for other digits

                    worldMap[y * mapWidth + x] = tileValue;
                }
            }

            auto world = CreateGameObject("World");
            auto map = world->AddComponent<TileMap>();
            map->LoadFromArray(worldMap.data(), mapWidth, mapHeight);

            auto renderer = world->AddComponent<TileMapRenderer>();
            renderer->wallColorNS = Color(0.8f, 0.2f, 0.2f, 1.0f);
            renderer->wallColorEW = Color(0.6f, 0.15f, 0.15f, 1.0f);
            renderer->ceilingColor = Color(0.1f, 0.1f, 0.2f, 1.0f);
            renderer->floorColor = Color(0.3f, 0.25f, 0.2f, 1.0f);

            renderer->SetCeilingSprite(AssetManager::GetSprite("Assets/Tilemap/modern_school_ceiling.png"));
            renderer->SetFloorSprite(AssetManager::GetSprite("Assets/Tilemap/modern_school_floor.png"));
            renderer->SetWallSprite(AssetManager::GetSprite("Assets/Tilemap/modern_school_wall.png"));

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

            auto playerHUD = player->AddComponent<PlayerHUD>();
            playerHUD->SetTileMap(map);
            playerHUD->SetPlayerTransform(playerTf);

            playerTf->position = Vector3(2.5f, 2.5f, 0.0f);
            playerTf->rotation = 0.0f;

            playerViewPos = playerTf->position;
            playerViewRot = playerTf->rotation;

            auto interactionCtrl = player->AddComponent<InteractionController>();
            interactionCtrl->interactKey = KeyCode::E;

            auto exit = CreateGameObject("Exit");
            exit->transform->position = exitPos;

            auto exitSR = exit->AddComponent<SpriteRenderer>();
            exitSR->SetSprite(AssetManager::GetSprite("Assets/PlanetTexture.png"));
            exitSR->billboard = true;

            auto enemy = CreateGameObject("SecurityGuard");
            enemy->transform->position = Vector2(5.5f, 5.5f);

            auto guard = enemy->AddComponent<SecurityGuard>();
            guard->tileMap = map;
            guard->playerTf = playerTf;
            guard->playerData = playerData;
            guard->patrolSpeed = 1.5f;
            guard->chaseSpeed = 2.4f;
            guardComp = guard;

            auto enemyRb = enemy->AddComponent<Rigidbody2D>();
            enemyRb->bodyType = RigidbodyType2D::Dynamic;
            enemyRb->gravityScale = 0.0f;
            enemyRb->drag = 0.0f;

            auto enemyCol = enemy->AddComponent<CircleCollider2D>();
            enemyCol->radius = 0.3f;
            enemyCol->minHeight = 0.0f;
            enemyCol->maxHeight = 2.0f;

            auto enemySR = enemy->AddComponent<SpriteRenderer>();
            enemySR->SetSprite(AssetManager::GetSprite("Assets/Brocsama.png"));
            enemySR->billboard = true;
            enemySR->sortingOrder = 100;

            playerHUD->AddEnemyTransform(enemy->transform);

            Sprite* powerup1 = AssetManager::GetSprite("Assets/powerup1.png");
            Sprite* powerup2 = AssetManager::GetSprite("Assets/powerup2.png");
            Sprite* powerup3 = AssetManager::GetSprite("Assets/powerup3.png");
            Sprite* powerup4 = AssetManager::GetSprite("Assets/powerup4.png");
            Sprite* powerup5 = AssetManager::GetSprite("Assets/powerup5.png");

            const float PU_Z = 0.20f;
            const int PU_ORDER = 300;

            auto SpawnPU = [&](char const* name, float x, float y, Sprite* spr) {
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

            Sprite* cctvSprite = AssetManager::GetSprite("Assets/cctv.png");

            const int DESIRED_CCTVS = 10;

            auto cams = GenerateCCTVs(map, mapWidth, mapHeight, 10, 7, 10.0f, 0.85f, 0.45f);


            for (int i = 0; i < (int)cams.size(); ++i)
            {
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
            if (Input::GetKeyDown(KeyCode::Alpha7)) ToggleCCTV(9);
            if (Input::GetKeyDown(KeyCode::Alpha8)) ToggleCCTV(10);
            if (Input::GetKeyDown(KeyCode::Alpha9)) ToggleCCTV(11);

            if (Input::GetKeyDown(KeyCode::Alpha0)) SwitchToPlayerCam();

            if (!playerData || !playerTf) return;
            if (playerData->GetState() != PlayerState::Normal) return;

            Vector2 p(playerTf->position.x, playerTf->position.y);
            float dx = exitPos.x - p.x;
            float dy = exitPos.y - p.y;

            if (guardComp) {
                for (auto* s : cctvSensors) {
                    if (s && s->hasLastSeen) {
                        guardComp->OnCCTVSpotted(s->lastSeenWorld);
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
                Application::Quit();
            }
        }
    };

} // namespace Game
