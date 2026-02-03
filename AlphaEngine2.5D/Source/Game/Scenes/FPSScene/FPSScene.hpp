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

        Vector2 exitPos = Vector2(122.5f, 38.5f);
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
            modalTitle->rectTransform.anchorMin = { 0.0f, 1.0f };
            modalTitle->rectTransform.anchorMax = { 1.0f, 1.0f };
            modalTitle->rectTransform.pivot = { 0.5f, 1.0f };
            modalTitle->rectTransform.anchoredPosition = { 0.0f, -24.0f };
            modalTitle->rectTransform.sizeDelta = { 0.0f, 64.0f };
            modalTitle->SetText("");
            modalTitle->SetFontSize(3.0f);
            modalTitle->SetColor(Engine::Color::white());
            modalTitle->SetAnchor(UI::TextAnchor::MiddleCenter);

            modalButton = modalPanel->CreateChild<UI::Button>();
            modalButton->name = "EndButton";
            modalButton->rectTransform.anchorMin = { 0.5f, 0.0f };
            modalButton->rectTransform.anchorMax = { 0.5f, 0.0f };
            modalButton->rectTransform.pivot = { 0.5f, 0.0f };
            modalButton->rectTransform.anchoredPosition = { 0.0f, 20.0f };
            modalButton->rectTransform.sizeDelta = { 240.0f, 56.0f };
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

            auto cams = GenerateCCTVs(map, mapWidth, mapHeight, 20, 5, 10.0f, 0.85f, 0.45f);


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
                playerHUD->AddCCTVTransform(camObj->transform);
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
                ShowEndPanel("YOU ESCAPED. WIN!", "Quit", []() {
                    SceneManager::LoadScene("FPSScene");
                    });
            }
        }
    };

} // namespace Game
