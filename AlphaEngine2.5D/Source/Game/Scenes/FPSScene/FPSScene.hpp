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
#include "Game/Player/Player.hpp"
#include "Game/SecurityGuard/SecurityGuard.hpp"
#include "Game/CCTV/CCTVCamera.hpp"
#include "Game/Scenes/FPSScene/UI/PlayerHUD.hpp"
#include "Game/Puzzle/PuzzleManager.h"
#include "Game/CCTV/CCTVPlacement.hpp"
#include "Game/NPC/NPC.hpp"
#include "Game/NPC/CameraShake.hpp"
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
        UI::Canvas* modalCanvas = nullptr;
        UI::Panel* modalPanel = nullptr;
        UI::Text* modalTitle = nullptr;
        UI::Button* modalButton = nullptr;
        bool endPanelShown = false;

        struct Zone {
            int id;
            int minX, maxX; 
            int minY, maxY;
        };

        std::vector<Zone> zones;

        int GetZoneId(int tx, int ty) const {
            for (auto const& z : zones) {
                if (tx >= z.minX && tx <= z.maxX &&
                    ty >= z.minY && ty <= z.maxY) {
                    return z.id;
                }
            }
            return -1;
        }

        const Zone* GetZone(int zoneId) const {
            for (auto const& z : zones) {
                if (z.id == zoneId) return &z;
            }
            return nullptr;
        }

        
        bool PickRandomWalkableInZone(const Zone& z, Engine::TileMap* map, int& outX, int& outY) const {
            if (!map) return false;

            for (int tries = 0; tries < 400; ++tries) {
                int x = z.minX + (std::rand() % (z.maxX - z.minX + 1));
                int y = z.minY + (std::rand() % (z.maxY - z.minY + 1));

                if (map->IsWalkable(x + 0.5f, y + 0.5f)) {
                    outX = x;
                    outY = y;
                    return true;
                }
            }
            return false;
        }

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

        // Create a hidden modal used for win/lose. Registers pause when shown.
        void CreateEndPanel() {
            if (!UI::UIManager::GetInstance()) {
                std::cout << "[Panel] Unable to create" << std::endl;
                return;
            }
            std::cout << "[Panel] creating" << std::endl;

            modalCanvas = UI::UIManager::GetInstance()->CreateCanvas();
            modalCanvas->SetSortOrder(1000);
            modalCanvas->SetReferenceResolution(1280.f, 720.f);
            modalCanvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);

            modalPanel = modalCanvas->CreateChild<UI::Panel>();
            modalPanel->name = "EndPanel";
            modalPanel->rectTransform.SetCentered(1000.0f, 560.0f);
            modalPanel->SetBackgroundColor(Engine::Color(0.0f, 0.0f, 0.0f, 1.0f));
            modalPanel->visible = false;

            modalTitle = modalPanel->CreateChild<UI::Text>();
            modalTitle->name = "EndTitle";
            modalTitle->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
            modalTitle->SetPivot({ 0.5f, 1.0f });
            modalTitle->SetAnchoredPosition({ 0.0f, -24.0f });
            modalTitle->SetSizeDelta({ 0.0f, 64.0f });
            modalTitle->SetText("");
            modalTitle->SetFontSize(3.0f);
            modalTitle->SetColor(Engine::Color::white());
            modalTitle->SetAnchor(UI::TextAnchor::MiddleCenter);

            modalButton = modalPanel->CreateChild<UI::Button>();
            modalButton->name = "EndButton";
            modalButton->SetAnchors({ 0.5f, 0.0f }, { 0.5f, 0.0f });
            modalButton->SetPivot({ 0.5f, 0.0f });
            modalButton->SetAnchoredPosition({ 0.0f, 20.0f });
            modalButton->SetSizeDelta({ 240.0f, 56.0f });
            modalButton->SetText("Restart");
            modalButton->SetBackgroundColor(Engine::Color(0.25f, 0.5f, 0.25f, 1.0f));

            // default behavior (will be overridden when showing with specific callback)
            modalButton->onClick = [this]() {
                // ensure pause is cleared before restarting
                Application::UnregisterPause("Endpanel");
                SceneManager::LoadScene("FPSScene");
                };
        }

        void ShowEndPanel(const std::string& titleText, const std::string& buttonText, std::function<void()> onButton) {
            if (!modalPanel || !modalTitle || !modalButton) return;

            modalTitle->SetText(titleText.c_str());
            modalButton->SetText(buttonText.c_str());
            modalPanel->visible = true;
            endPanelShown = true;

            std::cout << "[Panel] shown" << std::endl;

            // unlock cursor so user can interact with UI
            if (Input::IsCursorLocked()) {
                std::cout << "[Panel] releasing FPSController cursor lock\n";
                Input::UnregisterCursorLock("FPSController");
            }

            // register pause so Time.deltaTime == 0 and gameplay effectively pauses
            Application::RegisterPause("Endpanel");

            // replace onClick handler: first unregister pause, then invoke provided callback
            modalButton->onClick = [onButton, this]() {
                // re-acquire FPS cursor lock (only if the FPSController still exists)
                std::cout << "[Panel] re-registering FPSController cursor lock\n";
                Input::RegisterCursorLock("FPSController");

                Application::UnregisterPause("Endpanel");
                if (onButton) onButton();
                modalPanel->visible = false;
                endPanelShown = false;
                Application::Quit();
                };

            gameEnded = true;
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

            zones.clear();

            int midX = mapWidth / 2;
            int midY = mapHeight / 2;

            // 0: Top-left
            zones.push_back({ 1, 0, midX - 1, 0, midY - 1});

            // 1: Top-right
            zones.push_back({ 2, midX, mapWidth - 1, 0, midY - 1});

            // 2: Bottom-left
            zones.push_back({ 3, 0, midX - 1, midY, mapHeight - 1});

            // 3: Bottom-right
            zones.push_back({ 4, midX, mapWidth - 1, midY, mapHeight - 1});

            std::cout << "[ZONE] Created " << zones.size() << " zones\n";

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
            auto* camShake = player->AddComponent<Game::CameraShake>();
            // Enable fog for atmospheric effect
            playerCam->backgroundColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
            playerCam->fogEnabled = true;
            playerCam->fogDistance = 2.f;  // Fade over 8 units
            playerCam->fogMinVisibility = -0.1f;  // Slight visibility at max distance
            playerCam->fogMaxVisibility = 0.6f;  // Full brightness close up
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
            playerHUD->SetZoneGetter([this](int tx, int ty) {
                return this->GetZoneId(tx, ty);
                });
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

          
            auto* npcGO1 = CreateGameObject("NPC_Bully_Z1");
            {
                int sx = 0, sy = 0;
                const Zone* z = GetZone(1);
                if (z) PickRandomWalkableInZone(*z, map, sx, sy);
                npcGO1->transform->position = Vector2((float)sx + 0.5f, (float)sy + 0.5f);
            }

            auto* npc1 = npcGO1->AddComponent<Game::NPC>();
            npc1->tileMap = map;
            npc1->playerTf = playerTf;
            npc1->zoneId = 1;
            npc1->type = Game::NPCType::Bully;
            npc1->camShake = camShake;
            npc1->SetPickGoalInZone([this, map](int zid, int& gx, int& gy) -> bool {
                const Zone* z = this->GetZone(zid);
                if (!z) return false;
                return this->PickRandomWalkableInZone(*z, map, gx, gy);
                });

            auto* npcSR1 = npcGO1->AddComponent<SpriteRenderer>();
            npcSR1->SetSprite(AssetManager::GetSprite("Assets/xav.jpg"));
            npcSR1->billboard = true;
            npcSR1->sortingOrder = 95;

            auto* rb1 = npcGO1->AddComponent<Rigidbody2D>();
            rb1->bodyType = RigidbodyType2D::Kinematic;
            rb1->gravityScale = 0.0f;
            rb1->drag = 8.0f;

            auto* col1 = npcGO1->AddComponent<CircleCollider2D>();
            col1->radius = 0.25f;
            col1->isTrigger = false;
            col1->minHeight = 0.0f;
            col1->maxHeight = 2.0f;

            auto* npcGO2 = CreateGameObject("NPC_Cleaner_Z3");

            {
                int sx = 0, sy = 0;
                const Zone* z = GetZone(3);
                if (z) PickRandomWalkableInZone(*z, map, sx, sy);
                npcGO2->transform->position = Vector2((float)sx + 0.5f, (float)sy + 0.5f);
            }

            auto* npc2 = npcGO2->AddComponent<Game::NPC>();
            npc2->tileMap = map;
            npc2->playerTf = playerTf;
            npc2->zoneId = 3;
            npc2->type = Game::NPCType::Cleaner;
            npc2->speed = 0.8f;

            npc2->SetPickGoalInZone([this, map](int zid, int& gx, int& gy) -> bool {
                const Zone* z = this->GetZone(zid);
                if (!z) return false;
                return this->PickRandomWalkableInZone(*z, map, gx, gy);
                });

            auto* npcSR2 = npcGO2->AddComponent<SpriteRenderer>();
            npcSR2->SetSprite(AssetManager::GetSprite("Assets/xav.jpg"));
            npcSR2->billboard = true;
            npcSR2->sortingOrder = 95;

            auto* rb2 = npcGO2->AddComponent<Rigidbody2D>();
            rb2->bodyType = RigidbodyType2D::Kinematic;
            rb2->gravityScale = 0.0f;
            rb2->drag = 8.0f;

            auto* col2 = npcGO2->AddComponent<CircleCollider2D>();
            col2->radius = 0.25f;
            col2->isTrigger = false;
            col2->minHeight = 0.0f;
            col2->maxHeight = 2.0f;


        

            playerHUD->AddEnemyTransform(npcGO1->transform);
            playerHUD->AddEnemyTransform(npcGO2->transform);
            playerHUD->GetMap()->AddEnemyTransform(npcGO1->transform);
            playerHUD->GetMap()->AddEnemyTransform(npcGO2->transform);
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

            const float CCTV_Z = 0.85f;
            const float EPS = 0.02f;
            struct CCTVInfo { Vector3 pos; float rot; };
            CCTVInfo cams[] =
            {
                // LEFT side (face east)
                { Vector3(1.0f + EPS,  3.5f, CCTV_Z),   0.0f },
                { Vector3(1.0f + EPS, 13.5f, CCTV_Z),   0.0f },
                { Vector3(1.0f + EPS, 23.5f, CCTV_Z),   0.0f },
                { Vector3(1.0f + EPS, 35.5f, CCTV_Z),   0.0f },

                // RIGHT side (face west)  (max floor x in map.txt is 122)
                { Vector3(122.0f - EPS,  8.5f, CCTV_Z), 180.0f },
                { Vector3(122.0f - EPS, 18.5f, CCTV_Z), 180.0f },
                { Vector3(122.0f - EPS, 28.5f, CCTV_Z), 180.0f },
                { Vector3(122.0f - EPS, 36.5f, CCTV_Z), 180.0f },

                // TOP side (face south / down)
                { Vector3(10.5f,  1.0f + EPS, CCTV_Z),  90.0f },
                { Vector3(59.5f,  1.0f + EPS, CCTV_Z),  90.0f },
                { Vector3(110.5f,  1.0f + EPS, CCTV_Z),  90.0f },

                // BOTTOM side (face north / up)  (bottom corridor exists mainly on left)
                { Vector3(7.5f, 40.0f - EPS, CCTV_Z), 270.0f },
            };
            const int CCTV_COUNT = (int)(sizeof(cams) / sizeof(cams[0]));

            for (int i = 0; i < CCTV_COUNT; ++i)
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
                playerHUD->AddCCTVTransform(camObj->transform);
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
       
            CreateEndPanel();

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

            int camCount = (int)cctvViewPos.size();
            if (camCount > 0)
            {
                
                if (Input::GetKeyDown(KeyCode::DownArrow))
                {
                    if (activeCCTV < 0)
                        activeCCTV = 0;
                    else
                        activeCCTV = (activeCCTV + 1) % camCount;

                    SwitchToCCTV(activeCCTV);
                }

       
                if (Input::GetKeyDown(KeyCode::UpArrow))
                {
                    if (activeCCTV < 0)
                        activeCCTV = camCount - 1;
                    else
                        activeCCTV = (activeCCTV - 1 + camCount) % camCount;

                    SwitchToCCTV(activeCCTV);
                }
            }

  
            if (Input::GetKeyDown(KeyCode::Alpha0))
            {
                SwitchToPlayerCam();
          
            }

            if (!endPanelShown && playerData) {
                if (playerData->IsCaught()) {
                    std::cout << "[Panel] showing" << std::endl;
                    ShowEndPanel("YOU GOT CAUGHT. GAME OVER", "Quit", []() {
                        SceneManager::LoadScene("FPSScene");
                        });
                }
            }



            if (!playerData || !playerTf) return;
            if (playerData->GetState() != PlayerState::Normal) return;
            if (Engine::Input::GetKeyDown(Engine::KeyCode::Z)) {
                int tx = (int)playerTf->position.x;
                int ty = (int)playerTf->position.y;
                int zid = GetZoneId(tx, ty);
               
            }
          
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
