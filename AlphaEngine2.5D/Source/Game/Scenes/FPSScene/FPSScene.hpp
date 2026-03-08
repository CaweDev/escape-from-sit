#pragma once

#include "Engine/Scene/Scene.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Graphics/TileMapRenderer.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Physics/CircleCollider2D.hpp"
#include "Engine/Physics/BoxCollider2D.hpp"
#include "Engine/Physics/TileMapCollider2D.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/AssetManager.hpp"
#include "Engine/Input/KeyCode.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/TileMap/Door.hpp"
#include "Engine/Graphics/DoorRenderer.hpp"
#include "Engine/Interaction/Interactable.hpp"

#include "Game/FPSController.hpp"
#include "Game/Player/Player.hpp"
#include "Game/SecurityGuard/SecurityGuard.hpp"
#include "Game/CCTV/CCTVCamera.hpp"
#include "Game/CCTV/CCTVTerminal.hpp"
#include "Game/Scenes/FPSScene/UI/PlayerHUD.hpp"
#include "Game/Scenes/FPSScene/UI/PauseMenu.hpp"
#include "Game/Scenes/FPSScene/UI/Hands67.hpp"
#include "Game/Puzzle/PuzzleManager.h"
#include "Game/NPC/CameraShake.hpp"
#include <Engine/Graphics/SpriteRenderer.hpp>

#include "Game/ZoneManager/ZoneManager.hpp"
#include "Game/Heatmap/Heatmap.hpp"
#include "Game/UCB1Bandit/UCB1Bandit.hpp"
#include "Game/CCTV/CCTVPlacement.hpp"
#include "Game/LevelEditor/LevelEditor.hpp"
#include "Game/AccessCard/AccessCard.hpp"
#include "Game/ControlRoomDoor/ControlRoomDoor.hpp"
#include "Game/PathRenderer/PathRenderer.hpp"

#include "Engine/Interaction/InteractionController.hpp"

#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <fstream>
#include <functional>
#include <cstdlib>
#include <iomanip>

namespace Game {

    using namespace Engine;

    class FPSScene : public Scene {
    public:
        FPSScene() : Scene("FPSScene") {}

        // =====================
        // Core refs
        // =====================
        bool gameEnded = false;

        FPSController* playerController = nullptr;
        Player* playerData = nullptr;
        Transform* playerTf = nullptr;
        Camera* playerCam = nullptr;
        PlayerHUD* playerHUD = nullptr;

        // Guards / CCTV
        std::vector<Game::SecurityGuard*> guards;
        std::vector<Game::CCTVCamera*>    cctvSensors;
        std::vector<Game::CCTVTerminal*>  cctvTerminals;

        int activeCCTV = -1;
        std::vector<Vector3> cctvViewPos;
        std::vector<float>   cctvViewRot;

        Vector3 playerViewPos{};
        float   playerViewRot = 0.0f;

        // Exit
        Vector2 exitPos = Vector2(14.5f, 1.5f);
        float   winRadius = 0.6f;
        bool    printedWin = false;

        std::vector<GameObject*> powerUps;
        std::vector<float> powerUpBaseZ;
        float powerUpTime = 0.0f;
        struct CCTVInfo { Vector3 pos; float rotDeg; };
        PauseMenu* pauseMenu = nullptr;

        UI::Canvas* modalCanvas = nullptr;
        UI::Panel* modalPanel = nullptr;
        UI::Text* modalTitle = nullptr;
        UI::Button* modalButton = nullptr;
        bool endPanelShown = false;

        // =====================
        // Zones (4 quadrants)
        // =====================
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
                    return z.id; // 1..4
                }
            }
            return -1;
        }

        Vector2 GetZoneCenter(int zoneId) const {
            for (auto const& z : zones) {
                if (z.id == zoneId) {
                    float cx = (z.minX + z.maxX + 1) * 0.5f;
                    float cy = (z.minY + z.maxY + 1) * 0.5f;
                    return Vector2(cx, cy);
                }
            }
            return Vector2(0.0f, 0.0f);
        }

        // =====================
        // ML owner
        // =====================
        ZoneManager zoneMgr;
        Heatmap     heatmap;
        UCB1Bandit  bandit{ 4, 1.25f };

        std::vector<int> zoneGrid; // w*h, values 0..4

        float banditTimer = 0.0f;
        float banditInterval = 2.0f;

        int   prevChosenZoneId = 1; // 1..4 

        // Reward tuning
        float wHeat = 0.75f;
        float wCCTV = 0.25f;
        float heatRef = 50.0f; // tune based on testing
        float cctvRef = 6.0f;  // tune based on testing

        float heatAddPerSec = 1.0f;
        float heatDecayPerSec = 0.985f;

        // CCTV score with cooldown (prevents huge spikes)
        float cctvScore[5] = { 0,0,0,0,0 }; // use 1..4
        std::vector<float> cctvCooldown;   // per camera
        float cctvSpikeCooldown = 1.0f;    // seconds
        float cctvSpikeAmount = 1.0f;

        // "Nearest assist" behaviour
        int   priorityZoneId = 1;

        // Progressive escalation
        float priorityZoneHotTimer = 0.0f;
        float fullEscalationThreshold = 30.0f;
        bool  fullEscalationActive = false;

        // Debug (no flood)
        bool  debugML = false;
        float mlSummaryTimer = 0.0f;

        // Level Editor
        LevelEditor levelEditor;
        
        // Map dimensions (for debugging)
        int mapWidth = 0;
        int mapHeight = 0;

    private:
        static float Clamp01(float v) {
            if (v < 0.0f) return 0.0f;
            if (v > 1.0f) return 1.0f;
            return v;
        }

        void PrintMLSummary() {
            std::cout << "\n========== ML Summary ==========\n";
            std::cout << "Priority Zone: " << priorityZoneId << "\n";
            std::cout << "Hot Timer: " << priorityZoneHotTimer << " sec\n";
            std::cout << "Full Escalation: " << (fullEscalationActive ? "YES" : "NO") << "\n";
            std::cout << "Pulls: "
                << bandit.Pulls(0) << " "
                << bandit.Pulls(1) << " "
                << bandit.Pulls(2) << " "
                << bandit.Pulls(3) << "\n";
            std::cout << "Means: "
                << bandit.Value(0) << " "
                << bandit.Value(1) << " "
                << bandit.Value(2) << " "
                << bandit.Value(3) << "\n";
            std::cout << "CCTV Score: "
                << cctvScore[1] << " "
                << cctvScore[2] << " "
                << cctvScore[3] << " "
                << cctvScore[4] << "\n";

            // Show heatmap per zone
            for (int z = 1; z <= 4; ++z) {
                float heatSum = heatmap.SumZone(zoneMgr.GetGrid(), z);
                std::cout << "Zone " << z << " heat: " << heatSum << "\n";
            }
            std::cout << "================================\n";
        }

        void SwitchToPlayerCam() {
            activeCCTV = -1;
            if (!playerTf) return;
            playerTf->position = playerViewPos;
            playerTf->rotation = playerViewRot;
        }

        void SwitchToCCTV(int idx) {
            if (!playerTf) return;
            if (idx < 0 || idx >= (int)cctvViewPos.size()) return;
            activeCCTV = idx;
            playerTf->position = cctvViewPos[idx];
            playerTf->rotation = cctvViewRot[idx];
        }

        void CreateEndPanel() {
            if (!UI::UIManager::GetInstance()) return;

            modalCanvas = UI::UIManager::GetInstance()->CreateCanvas();
            modalCanvas->SetSortOrder(1000);
            modalCanvas->SetReferenceResolution(1280.f, 720.f);
            modalCanvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);

            modalPanel = modalCanvas->CreateChild<UI::Panel>();
            modalPanel->name = "EndPanel";
            modalPanel->rectTransform.SetCentered(1000.0f, 560.0f);
            modalPanel->SetBackgroundColor(Engine::Color(0, 0, 0, 1));
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

            modalButton->onClick = [this]() {
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

            if (Input::IsCursorLocked()) {
                Input::UnregisterCursorLock("FPSController");
            }

            Application::RegisterPause("Endpanel");

            modalButton->onClick = [onButton, this]() {
                Input::RegisterCursorLock("FPSController");
                Application::UnregisterPause("Endpanel");
                if (onButton) onButton();
                modalPanel->visible = false;
                endPanelShown = false;
                Application::Quit();
                };

            gameEnded = true;
        }

        // ===== Spawn one guard helper =====
        Game::SecurityGuard* SpawnGuardInZone(int zoneId, TileMap* map, PlayerHUD* hud, const char* spritePath)
        {
            int gx = 0, gy = 0;
            if (!zoneMgr.PickRandomWalkableInZone(zoneId, gx, gy)) {
                // fallback: just try near center
                Vector2 c = GetZoneCenter(zoneId);
                gx = (int)c.x;
                gy = (int)c.y;
            }

            auto go = CreateGameObject("Guard_Z" + std::to_string(zoneId));
            go->transform->position = Vector2((float)gx + 0.5f, (float)gy + 0.5f);

            auto guard = go->AddComponent<SecurityGuard>();
            guard->tileMap = map;
            guard->playerTf = playerTf;
            guard->playerData = playerData;
            guard->zoneMgr = &zoneMgr;
            guard->heatmap = &heatmap;

            guard->homeZoneId = zoneId;
            guard->SetPatrolZoneOverride(zoneId); // starts in its own zone

            auto rb = go->AddComponent<Rigidbody2D>();
            rb->bodyType = RigidbodyType2D::Dynamic;
            rb->gravityScale = 0.0f;
            rb->drag = 0.0f;

            auto col = go->AddComponent<CircleCollider2D>();
            col->radius = 0.3f;
            col->minHeight = 0.0f;
            col->maxHeight = 2.0f;

            auto sr = go->AddComponent<SpriteRenderer>();
            sr->SetSprite(AssetManager::GetSprite(spritePath));
            sr->billboard = true;
            sr->sortingOrder = 100;

            if (hud) hud->AddEnemyTransform(go->transform);

            guards.push_back(guard);
            return guard;
        }

        // ===== Progressive Escalation: nearest guard assists  full convergence =====
        void ApplyPriorityZone_AssistNearest(int pZone)
        {
            priorityZoneId = pZone;

            // Check if any guard in priority zone is chasing/searching
            // EXCLUDE guards investigating sabotage from escalation triggers
            bool anyGuardEngaged = false;
            for (auto* g : guards) {
                if (!g) continue;
                if (g->homeZoneId == pZone || g->GetPatrolZoneOverride() == pZone) {
                    // Don't count sabotage investigation as engagement
                    if (g->investigatingSabotage) continue;
                    
                    if (g->IsInChaseState() || g->IsInSearchState()) {
                        anyGuardEngaged = true;
                        break;
                    }
                }
            }

            // If guard engaged  trigger full escalation immediately
            if (anyGuardEngaged) {
                priorityZoneHotTimer = fullEscalationThreshold;
            }

            // Check if we should activate full escalation
            if (priorityZoneHotTimer >= fullEscalationThreshold && !fullEscalationActive) {
                fullEscalationActive = true;
                std::cout << "\n[ALERT] FULL ESCALATION! All guards converging on Zone " << pZone << "\n";

                // ALL guards shift to priority zone
                for (auto* g : guards) {
                    if (!g) continue;
                    if (g->IsInPatrolState()) {
                        g->SetFullEscalation(pZone);
                    }
                }
            }
            else if (!fullEscalationActive) {
                // Normal assist mode: only nearest guard helps
                for (auto* g : guards) {
                    if (!g) continue;
                    if (g->IsInPatrolState()) {
                        g->ClearAssist();
                        g->ClearFullEscalation();
                    }
                }

                Vector2 center = GetZoneCenter(pZone);
                SecurityGuard* nearest = nullptr;
                float bestD2 = 1e30f;

                for (auto* g : guards) {
                    if (!g) continue;
                    if (!g->IsInPatrolState()) continue;

                    Vector2 gp(g->transform->position.x, g->transform->position.y);
                    float dx = gp.x - center.x;
                    float dy = gp.y - center.y;
                    float d2 = dx * dx + dy * dy;

                    if (d2 < bestD2) {
                        bestD2 = d2;
                        nearest = g;
                    }
                }

                if (nearest) {
                    nearest->SetAssistZone(pZone);
                }
            }
        }

        void BanditTick()
        {
            // Observe player's actual current zone
            int ptx = (int)std::floor(playerTf->position.x);
            int pty = (int)std::floor(playerTf->position.y);
            int observedZone = GetZoneId(ptx, pty);
            if (observedZone < 1 || observedZone > 4) observedZone = 1; // fallback

            // Compute reward for the zone player is currently in
            float heatSum = heatmap.SumZone(zoneMgr.GetGrid(), observedZone);
            float heatN = Clamp01(heatSum / heatRef);

            float cctvN = Clamp01(cctvScore[observedZone] / cctvRef);

            float reward = Clamp01(wHeat * heatN + wCCTV * cctvN);

            // Update the bandit arm corresponding to observed zone
            bandit.Update(observedZone - 1, reward);

            // Choose next priority zone
            int nextArm = bandit.SelectArm();
            int nextZone = nextArm + 1;

            // Debug: Log bandit decision (only when it changes)
            static int lastDebugZone = -1;
            if (nextZone != lastDebugZone) {
                std::cout << "[BANDIT] Player in Zone " << Game::ZoneIdToLetter(observedZone) 
                          << " (" << observedZone << "), ML Priority ? Zone " << Game::ZoneIdToLetter(nextZone)
                          << " (" << nextZone << ")\n";
                std::cout << "  Heat: Z1=" << heatmap.SumZone(zoneMgr.GetGrid(), 1)
                          << " Z2=" << heatmap.SumZone(zoneMgr.GetGrid(), 2)
                          << " Z3=" << heatmap.SumZone(zoneMgr.GetGrid(), 3)
                          << " Z4=" << heatmap.SumZone(zoneMgr.GetGrid(), 4) << "\n";
                std::cout << "  CCTV: Z1=" << cctvScore[1] << " Z2=" << cctvScore[2]
                          << " Z3=" << cctvScore[3] << " Z4=" << cctvScore[4] << "\n";
                std::cout << "  Pulls: Arm0=" << bandit.Pulls(0) << " Arm1=" << bandit.Pulls(1)
                          << " Arm2=" << bandit.Pulls(2) << " Arm3=" << bandit.Pulls(3) << "\n";
                lastDebugZone = nextZone;
            }

            // Track how long zone has been priority
            if (nextZone == priorityZoneId) {
                priorityZoneHotTimer += banditInterval;
            } else {
                priorityZoneHotTimer = 0.0f; // reset if zone changes
                fullEscalationActive = false; // clear full escalation
            }

            prevChosenZoneId = nextZone;
            ApplyPriorityZone_AssistNearest(nextZone);

            // decay CCTV score gradually
            for (int z = 1; z <= 4; ++z) 
                cctvScore[z] *= 0.70f;
        }

    public:
        void OnLoad() override
        {
            // ===== load map.txt =====
            std::vector<std::string> mapRows;
            {
                std::ifstream file("Assets/map.txt");
                if (!file.is_open()) return;

                std::string line;
                while (std::getline(file, line)) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    if (!line.empty()) mapRows.push_back(line);
                }
                file.close();
                if (mapRows.empty()) return;
            }

            const int mapHeight = (int)mapRows.size();
            const int mapWidth = (int)mapRows[0].size();
            
            // Store for later use
            this->mapWidth = mapWidth;
            this->mapHeight = mapHeight;

            for (int y = 0; y < mapHeight; ++y) {
                if ((int)mapRows[y].size() != mapWidth) return;
            }

            // ===== zones (quadrants) =====
            // IMPORTANT: Minimap Y-axis is INVERTED from game coordinates!
            // Game coords: Y=0 is TOP, Y=max is BOTTOM
            // Minimap:     Y=0 is BOTTOM, Y=max is TOP
            // Zone layout (matching minimap colors when Y is INVERTED):
            //   Minimap Top-Left (Green):    Zone B (Game coords: Y=21-42, bottom-left)
            //   Minimap Top-Right (Magenta): Zone C (Game coords: Y=21-42, bottom-right)
            //   Minimap Bot-Left (Blue):     Zone A (Game coords: Y=0-20, top-left)
            //   Minimap Bot-Right (Yellow):  Zone D (Game coords: Y=0-20, top-right)
            zones.clear();
            int midX = mapWidth / 2;
            int midY = mapHeight / 2;

            zones.push_back({ 1, 0,        midX - 1, 0,        midY - 1 });      // A: Blue (Game top-left, Minimap bottom-left)
            zones.push_back({ 2, 0,        midX - 1, midY,     mapHeight - 1 }); // B: Green (Game bottom-left, Minimap top-left)
            zones.push_back({ 3, midX,     mapWidth - 1, midY, mapHeight - 1 }); // C: Magenta (Game bottom-right, Minimap top-right)
            zones.push_back({ 4, midX,     mapWidth - 1, 0,    midY - 1 });      // D: Yellow (Game top-right, Minimap bottom-right)

            // ===== build tile array =====
            std::vector<int> worldMap(mapWidth * mapHeight, 0);
            for (int y = 0; y < mapHeight; ++y) {
                for (int x = 0; x < mapWidth; ++x) {
                    char c = mapRows[y][x];
                    int tileValue = 0;

                    if (c == '0') tileValue = 0;       // Floor
                    else if (c == '1') tileValue = 1;  // Wall
                    else if (c == 'D') tileValue = 10; // Door
                    else if (c == 'T') tileValue = 11; // Terminal
                    else if (c == 'E') tileValue = 12; // Exit
                    else if (c == 'I') tileValue = 13; // Interactable
                    else if (c == 'C') tileValue = 13; // Control Room Door
                    else if (c == 'F') tileValue = 15; // Green Access Card
                    else if (c == 'G') tileValue = 16; // Purple Access Card
                    else if (c == 'H') tileValue = 17; // Red Access Card
                    else if (c == 'J') tileValue = 18; // Yellow Access Card
                    else tileValue = c - '0';          // Fallback numeric

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

            renderer->SetCeilingSprite(AssetManager::GetSprite("Assets/Tilemap/Wallpaper_02_White_1.png"));
            renderer->SetFloorSprite(AssetManager::GetSprite("Assets/Tilemap/s2 Concrete Squares 2 Grey.png"));
            renderer->SetWallSprite(AssetManager::GetSprite("Assets/Tilemap/Concrete_01_White_1.png"));
            renderer->SetDoorSprite(AssetManager::GetSprite("Assets/Tilemap/school_sliding_door.png"));

            auto tileMapCollider = world->AddComponent<TileMapCollider2D>();
            tileMapCollider->minHeight = 0.0f;
            tileMapCollider->maxHeight = 3.0f;

            // ===== player =====
            auto player = CreateGameObject("Player");
            playerCam = player->AddComponent<Camera>();
            playerCam->backgroundColor = Color(0, 0, 0, 1);
            playerCam->fogEnabled = true;
            playerCam->fogDistance = 2.f;
            playerCam->fogMinVisibility = -0.1f;
            playerCam->fogMaxVisibility = 0.6f;

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

            auto interactionCtrl = player->AddComponent<InteractionController>();
            interactionCtrl->interactKey = KeyCode::E;

            playerHUD = player->AddComponent<PlayerHUD>();
            playerHUD->SetTileMap(map);
            playerHUD->SetPlayerTransform(playerTf);
            playerHUD->SetZoneGetter([this](int tx, int ty) { return this->GetZoneId(tx, ty); });
            playerHUD->SetHeatmap(&heatmap); // Connect heatmap data

            pauseMenu = player->AddComponent<PauseMenu>();
            player->AddComponent<Hands67>();

            playerTf->position = Vector3(2.5f, 2.5f, 0.0f);
            playerTf->rotation = 0.0f;
            playerViewPos = playerTf->position;
            playerViewRot = playerTf->rotation;

            // ===== ML init =====
            zoneMgr.Init(map, mapWidth, mapHeight);
            heatmap.Init(mapWidth, mapHeight);

            zoneGrid.assign(mapWidth * mapHeight, 0);
            for (int y = 0; y < mapHeight; ++y) {
                for (int x = 0; x < mapWidth; ++x) {
                    int zid = GetZoneId(x, y);
                    zoneGrid[y * mapWidth + x] = (zid > 0) ? zid : 0;
                }
            }
            zoneMgr.SetZoneGrid(zoneGrid);

            // ===== spawn 4 guards (1 per zone) =====
            guards.clear();
            SpawnGuardInZone(1, map, playerHUD, "Assets/Brocsama.png");
            SpawnGuardInZone(2, map, playerHUD, "Assets/Brocsama.png");
            SpawnGuardInZone(3, map, playerHUD, "Assets/Brocsama.png");
            SpawnGuardInZone(4, map, playerHUD, "Assets/Brocsama.png");

            // start with zone 1 priority
            prevChosenZoneId = 1;
            ApplyPriorityZone_AssistNearest(prevChosenZoneId);

            // ===== CCTV placement =====
            Sprite* cctvSprite = AssetManager::GetSprite("Assets/cctv.png");
            const float CCTV_Z = 0.85f;
            const float EPS = 0.02f;

            // Place 8-12 cameras per zone for substantial corridor coverage
            // Reduced minDist to 3.5f to allow denser placement
            auto placements = Game::GenerateCCTVsPerZone(
                map, zoneGrid, mapWidth, mapHeight,
                10,    // perZone: 10 cameras per zone (40 total)
                1,     // step: check every tile (not used anymore with new algorithm)
                3.5f,  // minDist: minimum 3.5 tiles apart within same zone
                CCTV_Z, EPS
            );

            for (int i = 0; i < (int)placements.size(); ++i)
            {
                auto& p = placements[i];

                Vector3 view = p.pos;
                view.z = 1.6f;
                cctvViewPos.push_back(view);
                cctvViewRot.push_back(p.rot);

                auto camObj = CreateGameObject("CCTV_" + std::to_string(i + 1));
                camObj->transform->position = p.pos;
                camObj->transform->rotation = p.rot;
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
                sensor->hud = playerHUD;
                sensor->minimap = playerHUD->GetMinimap(); // Connect to minimap for vision cones
                sensor->detectRadius = 3.0f;
                sensor->maxRange = 6.0f;
                sensor->coneWidth = 70.0f;
                sensor->needLOS = true;
                sensor->spriteRenderer = sr; // Store sprite renderer for color changes
                
                printf("[CCTV Setup] Camera %d at (%.1f, %.1f) rotation=%.1f cone=%.1f range=%.1f\n",
                       i+1, p.pos.x, p.pos.y, p.rot, sensor->coneWidth, sensor->maxRange);

                cctvSensors.push_back(sensor);
            }

            cctvCooldown.assign(cctvSensors.size(), 0.0f);

            // ===== CCTV Terminals - Spawn from map markers ('T' in map.txt) =====
            Sprite* terminalSprite = AssetManager::GetSprite("Assets/powerup3.png");
            
            // Get terminal positions from map
            auto terminalPositions = map->GetTerminalPositions();
            
            if (terminalPositions.empty()) {
                std::cout << "[FPSScene] No terminal markers found in map. Terminals should be placed using the level editor.\n";
            }
            
            // Spawn terminals at marked positions
            for (size_t i = 0; i < terminalPositions.size(); ++i) {
                Vector2 pos = terminalPositions[i];
                
                // Determine zone from position (use floor to get tile coordinates)
                // Terminal positions are at tile center (x.5, y.5), so floor gives the tile
                int tileX = (int)std::floor(pos.x);
                int tileY = (int)std::floor(pos.y);
                int zoneId = GetZoneId(tileX, tileY);
                if (zoneId < 1 || zoneId > 4) {
                    std::cout << "[FPSScene] Warning: Terminal at (" << pos.x << ", " << pos.y 
                              << ") [tile " << tileX << "," << tileY << "] is outside valid zones. Skipping.\n";
                    continue;
                }
                
                std::cout << "[FPSScene] Terminal at (" << pos.x << ", " << pos.y 
                          << ") [tile " << tileX << "," << tileY << "] assigned to Zone " << zoneId << "\n";
                
                // Create terminal GameObject
                auto terminalObj = CreateGameObject("CCTVTerminal_Zone" + std::to_string(zoneId));
                terminalObj->transform->position = Vector3(pos.x, pos.y, 0.5f);
                terminalObj->transform->localScale = Vector2(0.5f, 0.5f);
                
                auto sr = terminalObj->AddComponent<SpriteRenderer>();
                sr->SetSprite(terminalSprite);
                sr->billboard = true;
                sr->sortingOrder = 250;
                sr->highlightColor = Engine::Color(0.3f, 0.3f, 0.0f, 0.0f); // Yellow glow
                
                auto terminal = terminalObj->AddComponent<CCTVTerminal>();
                terminal->controlledZone = zoneId;
                terminal->playerHUD = playerHUD;
                terminal->spriteRenderer = sr;
                terminal->allGuards = guards;
                
                // CCTVTerminal's SetupInteraction() is called in Awake()
                // It will automatically create and configure the Interactable component
                
                // Assign cameras to terminals based on zone
                for (size_t j = 0; j < cctvSensors.size(); ++j) {
                    auto* cam = cctvSensors[j];
                    if (!cam) continue;
                    
                    // Determine camera's zone
                    int camTx = (int)cam->transform->position.x;
                    int camTy = (int)cam->transform->position.y;
                    int camZone = GetZoneId(camTx, camTy);
                    
                    if (camZone == zoneId) {
                        terminal->controlledCameras.push_back(cam);
                    }
                }
                
                cctvTerminals.push_back(terminal);
                
                std::cout << "[FPSScene] Spawned terminal at (" << pos.x << ", " << pos.y 
                          << ") in Zone " << zoneId << " controlling " 
                          << terminal->controlledCameras.size() << " cameras\n";
            }
            
            // Register terminal positions with PlayerHUD for pathfinding
            for (size_t i = 0; i < terminalPositions.size(); ++i) {
                Vector2 pos = terminalPositions[i];
                int tileX = (int)std::floor(pos.x);
                int tileY = (int)std::floor(pos.y);
                int zoneId = GetZoneId(tileX, tileY);
                if (zoneId >= 1 && zoneId <= 4) {
                    playerHUD->SetTerminalPosition(zoneId, pos);
                }
            }

            // ===== Spawn Control Room Doors from map markers ('C' / tile 13) =====
            auto controlRoomDoorPositions = map->GetControlRoomDoorPositions();
            
            for (size_t i = 0; i < controlRoomDoorPositions.size(); ++i) {
                Vector2 pos = controlRoomDoorPositions[i];
                
                // Determine zone from position
                int tileX = (int)std::floor(pos.x);
                int tileY = (int)std::floor(pos.y);
                int zoneId = GetZoneId(tileX, tileY);
                if (zoneId < 1 || zoneId > 4) {
                    std::cout << "[FPSScene] Warning: Control room door at (" << pos.x << ", " << pos.y 
                              << ") is outside valid zones. Skipping.\n";
                    continue;
                }
                
                std::cout << "[FPSScene] Spawning control room door at (" << pos.x << ", " << pos.y 
                          << ") for Zone " << Game::ZoneIdToLetter(zoneId) << "\n";
                
                // Create door GameObject
                std::string doorName = "ControlRoomDoor_Zone" + std::to_string(zoneId);
                auto doorGO = CreateGameObject(doorName);
                doorGO->transform->position = Vector3(pos.x, pos.y, 0.0f);
                
                // Determine door facing based on surrounding walls
                bool wallNorth = map->IsWall(tileX, tileY + 1);
                bool wallSouth = map->IsWall(tileX, tileY - 1);
                Engine::DoorFacing facing = (wallNorth || wallSouth) ? Engine::DoorFacing::EastWest : Engine::DoorFacing::NorthSouth;
                
                // Add BoxCollider2D
                auto doorCollider = doorGO->AddComponent<Engine::BoxCollider2D>();
                doorCollider->minHeight = 0.0f;
                doorCollider->maxHeight = 2.0f;
                doorCollider->isTrigger = false;
                
                // Add Door component
                auto door = doorGO->AddComponent<Engine::Door>();
                door->tileX = tileX;
                door->tileY = tileY;
                door->doorType = 10;
                door->facing = facing;
                door->collider = doorCollider;
                door->state = Engine::DoorState::Closed;
                
                // Add DoorRenderer
                auto doorRenderer = doorGO->AddComponent<Engine::DoorRenderer>();
                doorRenderer->fov = renderer->fov;
                doorRenderer->sprite = renderer->GetDoorSprite();
                
                // Add Interactable
                auto interactable = doorGO->AddComponent<Engine::Interactable>();
                interactable->actionText = "LOCKED - Access Cards Required (0/4)";
                interactable->maxDistance = 2.5f;
                
                // Add ControlRoomDoor component
                auto controlDoor = doorGO->AddComponent<Game::ControlRoomDoor>();
                controlDoor->zoneId = zoneId;
                controlDoor->door = door;
                controlDoor->interactable = interactable;
            }
            
            // ===== Spawn Access Cards from map markers (tiles 15-18) =====
            auto accessCardPositions = map->GetAccessCardPositions();
            
            const char* cardSpritePaths[] = {
                "Assets/color_green_empty.png",
                "Assets/color_purple_empty.png",
                "Assets/color_red_empty.png",
                "Assets/color_yellow_empty.png"
            };
            
            for (size_t i = 0; i < accessCardPositions.size(); ++i) {
                Vector2 pos = accessCardPositions[i].first;
                int cardId = accessCardPositions[i].second;
                
                if (cardId < 0 || cardId > 3) continue;
                
                const char* cardNames[] = {"Green", "Purple", "Red", "Yellow"};
                
                // Create card GameObject
                std::string cardName = std::string(cardNames[cardId]) + "AccessCard";
                auto cardGO = CreateGameObject(cardName);
                cardGO->transform->position = Vector3(pos.x, pos.y, 0.5f);
                cardGO->transform->localScale = Vector2(0.4f, 0.4f);
                
                // Add SpriteRenderer
                auto sr = cardGO->AddComponent<Engine::SpriteRenderer>();
                sr->SetSprite(Engine::AssetManager::GetSprite(cardSpritePaths[cardId]));
                sr->billboard = true;
                sr->sortingOrder = 200;
                
                // Add Interactable
                auto interactable = cardGO->AddComponent<Engine::Interactable>();
                interactable->maxDistance = 2.5f;
                
                // Add AccessCard component
                auto card = cardGO->AddComponent<Game::AccessCard>();
                card->cardId = cardId;
                card->cardColor = static_cast<Game::AccessCardColor>(cardId);
                card->spriteRenderer = sr;
                card->interactable = interactable;
            }
            
            std::cout << "[FPSScene] Access card system initialized: " 
                      << controlRoomDoorPositions.size() << " doors, " 
                      << accessCardPositions.size() << " cards\n";

            // Connect terminals to guards
            for (auto* guard : guards) {
                if (guard) {
                    guard->SetTerminals(&cctvTerminals);
                }
            }

            // Add terminal icons to minimap
            if (playerHUD && playerHUD->GetMinimap()) {
                for (auto& pos : terminalPositions) {
                    playerHUD->GetMinimap()->AddTerminalIcon(
                        pos,
                        AssetManager::GetSprite("Assets/hill_top_smile.png")
                    );
                }
            }

            CreateEndPanel();
            SwitchToPlayerCam();

            // Initialize Level Editor
            levelEditor.Initialize(this, map, playerCam, playerTf);

            PuzzleManager::BookDataInit();
            PuzzleManager::Init();
        }

        void OnAfterRender() override
        {
            // Render level editor overlay on top of everything
            levelEditor.Render();
        }

        void OnUpdate() override
        {
            // Toggle Level Editor
            if (Input::GetKeyDown(KeyCode::F1)) {
                levelEditor.ToggleEditor();
            }

            // Update editor (only if active)
            levelEditor.Update();

            // If editor is active, skip game logic
            if (levelEditor.IsActive()) {
                return;
            }

            // Toggle ML debug summary (no flood)
            if (Input::GetKeyDown(KeyCode::M)) {
                debugML = !debugML;
                mlSummaryTimer = 0.0f;
            }

            // CCTV view switching (silent)
            int camCount = (int)cctvViewPos.size();
            if (camCount > 0) {
                if (Input::GetKeyDown(KeyCode::DownArrow)) {
                    if (activeCCTV < 0) activeCCTV = 0;
                    else activeCCTV = (activeCCTV + 1) % camCount;
                    SwitchToCCTV(activeCCTV);
                }
                if (Input::GetKeyDown(KeyCode::UpArrow)) {
                    if (activeCCTV < 0) activeCCTV = camCount - 1;
                    else activeCCTV = (activeCCTV - 1 + camCount) % camCount;
                    SwitchToCCTV(activeCCTV);
                }
            }
            if (Input::GetKeyDown(KeyCode::Alpha0)) {
                SwitchToPlayerCam();
            }

            // Lose panel
            if (!endPanelShown && playerData) {
                if (playerData->IsCaught()) {
                    ShowEndPanel("YOU GOT CAUGHT. GAME OVER", "Quit", []() {
                        SceneManager::LoadScene("FPSScene");
                        });
                }
            }

            if (!playerData || !playerTf) return;
            if (playerData->GetState() != PlayerState::Normal) return;
            
            // Update game timer
            playerData->UpdateGameTime(Engine::Time::deltaTime);
            
            // Update HUD elements
            if (playerHUD) {
                // Check zone changes for entry notification
                int tx = (int)std::floor(playerTf->position.x);
                int ty = (int)std::floor(playerTf->position.y);
                int currentZone = GetZoneId(tx, ty);
                
                // Debug: Print player position and zone (only on zone change)
                static int lastPrintedZone = -1;
                if (currentZone != lastPrintedZone) {
                    std::cout << "[FPSScene] Player at (" << tx << ", " << ty << ") ? Zone " << currentZone 
                              << " (" << Game::ZoneIdToLetter(currentZone) << ")" 
                              << " | Map size: " << mapWidth << "x" << mapHeight 
                              << " | Mid: " << (mapWidth/2) << "," << (mapHeight/2) << "\n";
                    lastPrintedZone = currentZone;
                }
                
                playerHUD->CheckZoneChange(currentZone);
                
                // Update stamina display
                playerHUD->UpdateStamina(playerData->GetStamina(), playerData->GetStaminaMax());
                
                // Update current time display
                playerHUD->UpdateCurrentTime(playerData->GetGameTime());
                
                // Update high score display
                playerHUD->UpdateHighScore(playerData->GetHighScore());
                
                // Find nearest guard for proximity warning and camera shake
                float nearestGuardDistance = 999999.0f;
                for (auto* guard : guards) {
                    if (!guard || !guard->transform) continue;
                    
                    Vector2 guardPos(guard->transform->position.x, guard->transform->position.y);
                    Vector2 playerPos(playerTf->position.x, playerTf->position.y);
                    
                    float dx = guardPos.x - playerPos.x;
                    float dy = guardPos.y - playerPos.y;
                    float distance = std::sqrt(dx * dx + dy * dy);
                    
                    if (distance < nearestGuardDistance) {
                        nearestGuardDistance = distance;
                    }
                }
                
                // Update guard proximity warning
                playerHUD->UpdateGuardProximity(nearestGuardDistance);
                
                // Apply camera shake based on guard proximity
                const float shakeDistance = 6.0f;
                if (nearestGuardDistance < shakeDistance && playerCam) {
                    auto cameraShake = playerCam->gameObject->GetComponent<CameraShake>();
                    if (!cameraShake) {
                        cameraShake = playerCam->gameObject->AddComponent<CameraShake>();
                        cameraShake->Initialize();
                    }
                    
                    // Intensity increases as guard gets closer
                    float intensity = (1.0f - (nearestGuardDistance / shakeDistance)) * 0.08f;
                    cameraShake->Shake(0.1f, intensity);
                }
                
                // Update alert level display
                int guardCount = 0;
                for (auto* g : guards) {
                    if (g && (g->IsInChaseState() || g->IsInSearchState())) {
                        guardCount++;
                    }
                }
                
                // Determine alert stage (1-4 based on escalation)
                int alertStage = 1;
                if (fullEscalationActive) alertStage = 4;
                else if (priorityZoneHotTimer > fullEscalationThreshold * 0.66f) alertStage = 3;
                else if (priorityZoneHotTimer > fullEscalationThreshold * 0.33f) alertStage = 2;
                
                playerHUD->UpdateAlertLevel(alertStage, priorityZoneId, guardCount);
            }

            // Heatmap update
            {
                int tx = (int)playerTf->position.x;
                int ty = (int)playerTf->position.y;

                heatmap.AddAt(tx, ty, heatAddPerSec * Engine::Time::deltaTime);

                float f = std::pow(heatDecayPerSec, Engine::Time::deltaTime);
                heatmap.Decay(f);
            }

            // CCTV spikes + notify guards (with cooldown)
            for (size_t i = 0; i < cctvSensors.size(); ++i)
            {
                auto* s = cctvSensors[i];
                if (!s) continue;

                if (cctvCooldown[i] > 0.0f)
                    cctvCooldown[i] -= Engine::Time::deltaTime;

                if (s->hasLastSeen && cctvCooldown[i] <= 0.0f)
                {
                    int tx = (int)s->lastSeenWorld.x;
                    int ty = (int)s->lastSeenWorld.y;
                    int zid = GetZoneId(tx, ty);
                    if (zid >= 1 && zid <= 4) {
                        cctvScore[zid] += cctvSpikeAmount;

                        // CCTV triggers immediate priority update (feels responsive)
                        priorityZoneId = zid;
                        ApplyPriorityZone_AssistNearest(zid);
                    }

                    // notify guards (Option B: only affects Patrol guards)
                    for (auto* g : guards)
                        if (g) g->OnCCTVSpotted(s->lastSeenWorld);

                    cctvCooldown[i] = cctvSpikeCooldown;
                }

                // clear every frame so it doesn't spam
                s->hasLastSeen = false;
            }

            // Bandit tick
            banditTimer += Engine::Time::deltaTime;
            if (banditTimer >= banditInterval) {
                banditTimer = 0.0f;
                BanditTick();
            }

            // ML summary every 5 sec only when debug on
            if (debugML) {
                mlSummaryTimer += Engine::Time::deltaTime;
                if (mlSummaryTimer >= 5.0f) {
                    mlSummaryTimer = 0.0f;
                    PrintMLSummary();
                }
            }

            // Win check
            Vector2 p(playerTf->position.x, playerTf->position.y);
            float dx = exitPos.x - p.x;
            float dy = exitPos.y - p.y;

            if (!printedWin && (dx * dx + dy * dy) <= winRadius * winRadius) {
                printedWin = true;
                gameEnded = true;
                
                // Update high score if this is a new record
                float currentTime = playerData->GetGameTime();
                if (currentTime < playerData->GetHighScore()) {
                    playerData->SetHighScore(currentTime);
                    std::cout << "NEW HIGH SCORE: " << (int)(currentTime / 60) << ":" 
                              << std::setfill('0') << std::setw(2) << ((int)currentTime % 60) << "!\n";
                }
                
                playerData->Win();
                std::cout << "YOU ESCAPED. WIN!\n";
                Application::Quit();
            }
        }
        void OnUnload() override
        {

        }
    };

  

} // namespace Game