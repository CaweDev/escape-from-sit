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
#include "Engine/Audio/SoundManager.hpp"
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
#include "Game/Scenes/FPSScene/UI/LoseScreen.hpp"
#include "Game/Scenes/FPSScene/UI/WinScreen.hpp"
#include "Game/Scenes/FPSScene/UI/Hands67.hpp"
#include "Game/Puzzle/PuzzleManager.h"
#include "Game/NPC/CameraShake.hpp"
#include "Game/NPC/NPC.hpp"
#include <Engine/Graphics/SpriteRenderer.hpp>
#include "Game/ZoneManager/ZoneManager.hpp"
#include "Game/Heatmap/Heatmap.hpp"
#include "Game/UCB1Bandit/UCB1Bandit.hpp"
#include "Game/UCB1Bandit/BanditVisualizer.hpp"
#include "Game/CCTV/CCTVPlacement.hpp"
#include "Game/LevelEditor/LevelEditor.hpp"
#include "Game/AccessCard/AccessCard.hpp"
#include "Game/ControlRoomDoor/ControlRoomDoor.hpp"
#include "Game/PathRenderer/PathRenderer.hpp"
#include "Game/PowerUp/PowerUpManager.h"
#include "Game/Player/PlayerPowerUpController.h"

#include "Engine/Interaction/InteractionController.hpp"
#include "Editor/ObjectManager.hpp"

#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <fstream>
#include <functional>
#include <cstdlib>
#include <iomanip>

namespace Game
{

    using namespace Engine;

    class FPSScene : public Scene
    {
    public:
        FPSScene() : Scene("FPSScene") {}

        // =====================
        // Core refs
        // =====================
        bool gameEnded = false;

        FPSController *playerController = nullptr;
        Player *playerData = nullptr;
        Transform *playerTf = nullptr;
        Camera *playerCam = nullptr;
        PlayerHUD *playerHUD = nullptr;
        PlayerPowerUpController *powerUpController = nullptr;

        // Guards / CCTV
        std::vector<Game::SecurityGuard *> guards;
        std::vector<Game::CCTVCamera *> cctvSensors;
        std::vector<Game::CCTVTerminal *> cctvTerminals;

        int activeCCTV = -1;
        std::vector<Vector3> cctvViewPos;
        std::vector<float> cctvViewRot;

        Vector3 playerViewPos{};
        float playerViewRot = 0.0f;

        // Exit
        Vector2 exitPos;
        float winRadius = 0.6f;
        bool printedWin = false;

        std::vector<GameObject *> powerUps;
        std::vector<float> powerUpBaseZ;
        float powerUpTime = 0.0f;
        struct CCTVInfo
        {
            Vector3 pos;
            float rotDeg;
        };
        PauseMenu *pauseMenu = nullptr;
        Game::LoseScreen *loseScreen = nullptr;
        Game::WinScreen *winScreen = nullptr;

        // UI::Canvas* modalCanvas = nullptr;
        // UI::Panel* modalPanel = nullptr;
        // UI::Text* modalTitle = nullptr;
        // UI::Button* modalButton = nullptr;
        // bool endPanelShown = false;

        // =====================
        // Zones (4 quadrants)
        // =====================
        struct Zone
        {
            int id;
            int minX, maxX;
            int minY, maxY;
        };
        std::vector<Zone> zones;

        int GetZoneId(int tx, int ty) const
        {
            for (auto const &z : zones)
            {
                if (tx >= z.minX && tx <= z.maxX &&
                    ty >= z.minY && ty <= z.maxY)
                {
                    return z.id; // 1..4
                }
            }
            return -1;
        }

        Vector2 GetZoneCenter(int zoneId) const
        {
            for (auto const &z : zones)
            {
                if (z.id == zoneId)
                {
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
        UCB1Bandit  bandit{ 4, 2.0f };

        std::vector<int> zoneGrid; // w*h, values 0..4

        float banditTimer = 0.0f;
        float banditInterval = 2.0f;

        int prevChosenZoneId = 1; // 1..4

        // Reward tuning
        float wHeat = 0.55f;
        float wCCTV = 0.45f;
        float heatRef = 50.0f; // tune based on testing
        float cctvRef = 6.0f;  // tune based on testing

        float heatAddPerSec = 1.0f;
        float heatDecayPerSec = 0.985f;

        // CCTV score with cooldown (prevents huge spikes)
        float cctvScore[5] = {0, 0, 0, 0, 0}; // use 1..4
        std::vector<float> cctvCooldown;      // per camera
        float cctvSpikeCooldown = 1.0f;       // seconds
        float cctvSpikeAmount = 1.0f;

        // "Nearest assist" behaviour
        int priorityZoneId = 1;

        // Progressive escalation
        float priorityZoneHotTimer = 0.0f;
        float fullEscalationThreshold = 30.0f;
        bool fullEscalationActive = false;

        // Debug (no flood)
        bool debugML = false;
        float mlSummaryTimer = 0.0f;

        // NPCs (Bully + Annoying classmates)
        std::vector<Game::NPC *> npcs;

        // Level Editor
        LevelEditor levelEditor;
        Game::BanditVisualizer* banditVisualizer = nullptr;
        
        // Map dimensions (for debugging)
        int mapWidth = 0;
        int mapHeight = 0;

    private:
        static float Clamp01(float v)
        {
            if (v < 0.0f)
                return 0.0f;
            if (v > 1.0f)
                return 1.0f;
            return v;
        }

        void PrintMLSummary()
        {
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
            for (int z = 1; z <= 4; ++z)
            {
                float heatSum = heatmap.SumZone(zoneMgr.GetGrid(), z);
                std::cout << "Zone " << z << " heat: " << heatSum << "\n";
            }
            std::cout << "================================\n";
        }

        void SwitchToPlayerCam()
        {
            activeCCTV = -1;
            if (!playerTf)
                return;
            playerTf->position = playerViewPos;
            playerTf->rotation = playerViewRot;
        }

        void SwitchToCCTV(int idx)
        {
            if (!playerTf)
                return;
            if (idx < 0 || idx >= (int)cctvViewPos.size())
                return;
            activeCCTV = idx;
            playerTf->position = cctvViewPos[idx];
            playerTf->rotation = cctvViewRot[idx];
        }

        /* void CreateEndPanel() {
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

             Input::RegisterCursorUnlock("EndPanel");

             Application::RegisterPause("Endpanel");

             modalButton->onClick = [onButton, this]() {
                 SoundManager::PlaySFX("ui_click");
                 Input::UnregisterCursorUnlock("EndPanel");
                 Application::UnregisterPause("Endpanel");
                 if (onButton) onButton();
                 modalPanel->visible = false;
                 endPanelShown = false;
                 Application::Quit();
                 };

             gameEnded = true;
         }*/

        // ===== Spawn one NPC helper =====
        // Player spawn position - NPCs must not spawn near here
        static constexpr float spawnExcludeX = 2.5f;
        static constexpr float spawnExcludeY = 2.5f;
        static constexpr float spawnExcludeRadius = 5.0f;

        Game::NPC *SpawnNPCInZone(int zoneId, NPCType npcType, TileMap *map, const char *spritePath)
        {
            int gx = 0, gy = 0;
            bool found = false;
            // Try multiple times to find a position away from the player spawn area
            for (int attempt = 0; attempt < 50; ++attempt)
            {
                if (!zoneMgr.PickRandomWalkableInZone(zoneId, gx, gy))
                    break;
                float dx = (float)gx + 0.5f - spawnExcludeX;
                float dy = (float)gy + 0.5f - spawnExcludeY;
                if (dx * dx + dy * dy > spawnExcludeRadius * spawnExcludeRadius)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                if (!zoneMgr.PickRandomWalkableInZone(zoneId, gx, gy))
                {
                    Vector2 c = GetZoneCenter(zoneId);
                    gx = (int)c.x;
                    gy = (int)c.y;
                }
            }

            const char *typeName = (npcType == NPCType::Bully) ? "Bully" : "Annoying";
            auto go = CreateGameObject(std::string(typeName) + "_Z" + std::to_string(zoneId));
            go->transform->position = Vector2((float)gx + 0.5f, (float)gy + 0.5f);

            auto npc = go->AddComponent<Game::NPC>();
            npc->tileMap = map;
            npc->playerTf = playerTf;
            npc->playerData = playerData;
            npc->type = npcType;
            npc->zoneId = zoneId;

            //    // Get CameraShake from player camera for bully push effect
            if (playerCam)
            {
                auto shake = playerCam->gameObject->GetComponent<CameraShake>();
                if (!shake)
                {
                    shake = playerCam->gameObject->AddComponent<CameraShake>();
                    shake->Initialize();
                }
                npc->camShake = shake;
            }

            //    // Let NPC pick random walkable goals inside its zone via ZoneManager
            npc->SetPickGoalInZone([this](int zId, int &outX, int &outY) -> bool
                                   { return zoneMgr.PickRandomWalkableInZone(zId, outX, outY); });

            //    // Tuning per type
            if (npcType == NPCType::Bully)
            {
                npc->speed = 2.0f;
                npc->triggerRadius = 3.0f;
            }
            else
            {
                npc->speed = 1.2f;
                npc->triggerRadius = 2.5f;
                npc->blockTime = 1.5f;
            }

            auto npcRb = go->AddComponent<Rigidbody2D>();
            npcRb->bodyType = RigidbodyType2D::Dynamic;
            npcRb->gravityScale = 0.0f;
            npcRb->drag = 0.0f;

            auto npcCol = go->AddComponent<CircleCollider2D>();
            npcCol->radius = 0.25f;
            npcCol->minHeight = 0.0f;
            npcCol->maxHeight = 2.0f;

            auto sr = go->AddComponent<SpriteSheetRenderer>();
            sr->SetSpriteSheet(AssetManager::GetSpriteSheet(spritePath));
            sr->billboard = true;
            sr->sortingOrder = 90;
            sr->Play("Patrol");

            npcs.push_back(npc);

            std::cout << "[FPSScene] Spawned " << typeName << " NPC in Zone " << zoneId
                      << " at (" << gx << ", " << gy << ")\n";

            return npc;
        }

        // ===== Spawn one guard helper =====
        Game::SecurityGuard *SpawnGuardInZone(int zoneId, TileMap *map, PlayerHUD *hud, const char *spritePath)
        {
            int gx = 0, gy = 0;
            if (!zoneMgr.PickRandomWalkableInZone(zoneId, gx, gy))
            {
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

            auto sr = go->AddComponent<SpriteSheetRenderer>();
            sr->SetSpriteSheet(AssetManager::GetSpriteSheet(spritePath));
            sr->billboard = true;
            sr->sortingOrder = 100;
            sr->Play("Patrol");

            if (hud)
                hud->AddEnemyTransform(go->transform);

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
            for (auto *g : guards)
            {
                if (!g)
                    continue;
                if (g->homeZoneId == pZone || g->GetPatrolZoneOverride() == pZone)
                {
                    // Don't count sabotage investigation as engagement
                    if (g->investigatingSabotage)
                        continue;

                    if (g->IsInChaseState() || g->IsInSearchState())
                    {
                        anyGuardEngaged = true;
                        break;
                    }
                }
            }

            // If guard engaged  trigger full escalation immediately
            if (anyGuardEngaged)
            {
                priorityZoneHotTimer = fullEscalationThreshold;
            }

            // Check if we should activate full escalation
            if (priorityZoneHotTimer >= fullEscalationThreshold && !fullEscalationActive)
            {
                fullEscalationActive = true;
                std::cout << "\n[ALERT] FULL ESCALATION! All guards converging on Zone " << pZone << "\n";

                // ALL guards shift to priority zone
                for (auto *g : guards)
                {
                    if (!g)
                        continue;
                    if (g->IsInPatrolState())
                    {
                        g->SetFullEscalation(pZone);
                    }
                }
            }
            else if (!fullEscalationActive)
            {
                // Normal assist mode: only nearest guard helps
                for (auto *g : guards)
                {
                    if (!g)
                        continue;
                    if (g->IsInPatrolState())
                    {
                        g->ClearAssist();
                        g->ClearFullEscalation();
                    }
                }

                Vector2 center = GetZoneCenter(pZone);
                SecurityGuard *nearest = nullptr;
                float bestD2 = 1e30f;

                for (auto *g : guards)
                {
                    if (!g)
                        continue;
                    if (!g->IsInPatrolState())
                        continue;

                    Vector2 gp(g->transform->position.x, g->transform->position.y);
                    float dx = gp.x - center.x;
                    float dy = gp.y - center.y;
                    float d2 = dx * dx + dy * dy;

                    if (d2 < bestD2)
                    {
                        bestD2 = d2;
                        nearest = g;
                    }
                }

                if (nearest)
                {
                    nearest->SetAssistZone(pZone);
                }
            }
        }
        void BanditTick()
        {
            float zoneReward[5] = { 0,0,0,0,0 };

            // Compute current reward signal for ALL zones
            for (int z = 1; z <= 4; ++z)
            {
                float heatSum = heatmap.SumZone(zoneMgr.GetGrid(), z);
                float heatN = Clamp01(heatSum / heatRef);

                float cctvN = Clamp01(cctvScore[z] / cctvRef);

                zoneReward[z] = Clamp01(wHeat * heatN + wCCTV * cctvN);
            }

            static float switchCooldown = 0.0f;
            switchCooldown -= banditInterval;
            if (switchCooldown < 0.0f) switchCooldown = 0.0f;

            int nextArm = bandit.SelectArm();
            if (nextArm < 0) return;

            int nextZone = nextArm + 1;

            // Stay committed to current priority while cooldown is active
            if (switchCooldown > 0.0f) {
                nextZone = priorityZoneId;
                nextArm = nextZone - 1;
            }
            else {
                if (nextZone != priorityZoneId) {
                    // Strong CCTV evidence -> stay longer
                    if (cctvScore[nextZone] > 0.5f) {
                        switchCooldown = 25.0f;
                    }
                    else {
                        switchCooldown = 8.0f;
                    }
                }
            }

            // Update chosen arm using that zone's current reward
            bandit.Update(nextArm, zoneReward[nextZone]);

            // Debug print
            static int lastDebugZone = -1;
            if (nextZone != lastDebugZone) {
                std::cout << "[BANDIT] ML Priority -> Zone "
                    << Game::ZoneIdToLetter(nextZone)
                    << " (" << nextZone << ")\n";

                std::cout << "  Reward: Z1=" << zoneReward[1]
                    << " Z2=" << zoneReward[2]
                    << " Z3=" << zoneReward[3]
                    << " Z4=" << zoneReward[4] << "\n";

                std::cout << "  Heat: Z1=" << heatmap.SumZone(zoneMgr.GetGrid(), 1)
                    << " Z2=" << heatmap.SumZone(zoneMgr.GetGrid(), 2)
                    << " Z3=" << heatmap.SumZone(zoneMgr.GetGrid(), 3)
                    << " Z4=" << heatmap.SumZone(zoneMgr.GetGrid(), 4) << "\n";

                std::cout << "  CCTV: Z1=" << cctvScore[1]
                    << " Z2=" << cctvScore[2]
                    << " Z3=" << cctvScore[3]
                    << " Z4=" << cctvScore[4] << "\n";

                std::cout << "  Pulls: Arm0=" << bandit.Pulls(0)
                    << " Arm1=" << bandit.Pulls(1)
                    << " Arm2=" << bandit.Pulls(2)
                    << " Arm3=" << bandit.Pulls(3) << "\n";

                std::cout << "  Means: Arm0=" << bandit.Value(0)
                    << " Arm1=" << bandit.Value(1)
                    << " Arm2=" << bandit.Value(2)
                    << " Arm3=" << bandit.Value(3) << "\n";

                std::cout << "  Commit cooldown: " << switchCooldown << "s\n";

                lastDebugZone = nextZone;
            }

            // Track how long chosen zone stays priority
            if (nextZone == priorityZoneId) {
                priorityZoneHotTimer += banditInterval;
            }
            else {
                priorityZoneHotTimer = 0.0f;
                fullEscalationActive = false;
            }

            prevChosenZoneId = nextZone;
            priorityZoneId = nextZone;
            ApplyPriorityZone_AssistNearest(nextZone);

            // Decay CCTV score less aggressively
            for (int z = 1; z <= 4; ++z)
                cctvScore[z] *= 0.85f;
        }
      

    public:
        void OnLoad() override
        {
            gameEnded = false;
            printedWin = false;
            SoundManager::PlayBGM("fps_gameplay", true);
            // ===== load map.txt =====
            std::vector<std::string> mapRows;
            {
                std::ifstream file("Assets/tmpMap.txt");
                if (!file.is_open())
                {
                    file.clear();
                    file.open("Assets/map.txt");
                }

                if (!file.is_open())
                    return;

                std::string line;
                while (std::getline(file, line))
                {
                    if (!line.empty() && line.back() == '\r')
                        line.pop_back();
                    if (!line.empty())
                        mapRows.push_back(line);
                }
                file.close();
                if (mapRows.empty())
                    return;
            }

            const int mapHeight = (int)mapRows.size();
            const int mapWidth = (int)mapRows[0].size();

            // Store for later use
            this->mapWidth = mapWidth;
            this->mapHeight = mapHeight;

            for (int y = 0; y < mapHeight; ++y)
            {
                if ((int)mapRows[y].size() != mapWidth)
                    return;
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

            zones.push_back({1, 0, midX - 1, 0, midY - 1});                // A: Blue (Game top-left, Minimap bottom-left)
            zones.push_back({2, 0, midX - 1, midY, mapHeight - 1});        // B: Green (Game bottom-left, Minimap top-left)
            zones.push_back({3, midX, mapWidth - 1, midY, mapHeight - 1}); // C: Magenta (Game bottom-right, Minimap top-right)
            zones.push_back({4, midX, mapWidth - 1, 0, midY - 1});         // D: Yellow (Game top-right, Minimap bottom-right)

            // ===== build tile array =====
            std::vector<int> worldMap(mapWidth * mapHeight, 0);
            for (int y = 0; y < mapHeight; ++y)
            {
                for (int x = 0; x < mapWidth; ++x)
                {
                    char c = mapRows[y][x];
                    int tileValue = 0;

                    if (c == '0')
                        tileValue = 0; // Floor
                    else if (c == '1')
                        tileValue = 1; // Wall
                    else if (c == 'D')
                        tileValue = 10; // Door
                    else if (c == 'T')
                        tileValue = 11; // Terminal
                    else if (c == 'E')
                        tileValue = 12; // Exit
                    else if (c == 'C')
                        tileValue = 13; // Control Room Door (old marker)
                    else if (c == 'K')
                        tileValue = 20; // Control Room Door (NEW dedicated tile)
                    else if (c == 'F')
                        tileValue = 15; // Green Access Card
                    else if (c == 'G')
                        tileValue = 16; // Purple Access Card
                    else if (c == 'H')
                        tileValue = 17; // Red Access Card
                    else if (c == 'J')
                        tileValue = 18; // Yellow Access Card
                    else
                        tileValue = c - '0'; // Fallback numeric

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
            playerHUD->SetZoneGetter([this](int tx, int ty)
                                     { return this->GetZoneId(tx, ty); });
            playerHUD->SetHeatmap(&heatmap); // Connect heatmap data

            pauseMenu = player->AddComponent<PauseMenu>();
            player->AddComponent<Hands67>();
            loseScreen = player->AddComponent<LoseScreen>();
            winScreen = player->AddComponent<WinScreen>();

            playerTf->position = Vector3(2.5f, 2.5f, 0.0f);
            playerTf->rotation = 0.0f;
            playerViewPos = playerTf->position;
            playerViewRot = playerTf->rotation;

            powerUpController = player->AddComponent<PlayerPowerUpController>();

            // ===== ML init =====
            zoneMgr.Init(map, mapWidth, mapHeight);
            heatmap.Init(mapWidth, mapHeight);

            zoneGrid.assign(mapWidth * mapHeight, 0);
            for (int y = 0; y < mapHeight; ++y)
            {
                for (int x = 0; x < mapWidth; ++x)
                {
                    int zid = GetZoneId(x, y);
                    zoneGrid[y * mapWidth + x] = (zid > 0) ? zid : 0;
                }
            }
            zoneMgr.SetZoneGrid(zoneGrid);

            // ===== Read exit position from map (fallback to default) =====
            auto mapExitPositions = map->GetExitPositions();
            if (!mapExitPositions.empty())
            {
                exitPos = mapExitPositions[0];
                std::cout << "[FPSScene] Exit position from map: (" << exitPos.x << ", " << exitPos.y << ")\n";
            }
            else
            {
                std::cout << "[FPSScene] No exit marker in map, using default: (" << exitPos.x << ", " << exitPos.y << ")\n";
            }

            // ===== spawn 4 guards (1 per zone) =====
            guards.clear();
            SpawnGuardInZone(1, map, playerHUD, "Assets/guard.png");
            SpawnGuardInZone(2, map, playerHUD, "Assets/guard.png");
            SpawnGuardInZone(3, map, playerHUD, "Assets/guard.png");
            SpawnGuardInZone(4, map, playerHUD, "Assets/guard.png");

            // start with zone 1 priority
            prevChosenZoneId = 1;
            ApplyPriorityZone_AssistNearest(prevChosenZoneId);

            // ===== CCTV placement =====
            Sprite *cctvSprite = AssetManager::GetSprite("Assets/cctv.png");
            const float CCTV_Z = 0.85f;
            const float EPS = 0.02f;

            // Place 8-12 cameras per zone for substantial corridor coverage
            // Reduced minDist to 3.5f to allow denser placement
            auto placements = Game::GenerateCCTVsPerZone(
                map, zoneGrid, mapWidth, mapHeight,
                10,   // perZone: 10 cameras per zone (40 total)
                1,    // step: check every tile (not used anymore with new algorithm)
                3.5f, // minDist: minimum 3.5 tiles apart within same zone
                CCTV_Z, EPS);

            for (int i = 0; i < (int)placements.size(); ++i)
            {
                auto &p = placements[i];

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
                       i + 1, p.pos.x, p.pos.y, p.rot, sensor->coneWidth, sensor->maxRange);

                cctvSensors.push_back(sensor);
            }

            cctvCooldown.assign(cctvSensors.size(), 0.0f);

            // ===== CCTV Terminals - Spawn from map markers ('T' in map.txt) =====
            Sprite *terminalSprite = AssetManager::GetSprite("Assets/powerup3.png");

            // Get terminal positions from map
            auto terminalPositions = map->GetTerminalPositions();

            if (terminalPositions.empty())
            {
                std::cout << "[FPSScene] No terminal markers found in map. Terminals should be placed using the level editor.\n";
            }

            // Spawn terminals at marked positions
            for (size_t i = 0; i < terminalPositions.size(); ++i)
            {
                Vector2 pos = terminalPositions[i];

                // Determine zone from position (use floor to get tile coordinates)
                // Terminal positions are at tile center (x.5, y.5), so floor gives the tile
                int tileX = (int)std::floor(pos.x);
                int tileY = (int)std::floor(pos.y);
                int zoneId = GetZoneId(tileX, tileY);
                if (zoneId < 1 || zoneId > 4)
                {
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
                for (size_t j = 0; j < cctvSensors.size(); ++j)
                {
                    auto *cam = cctvSensors[j];
                    if (!cam)
                        continue;

                    // Determine camera's zone
                    int camTx = (int)cam->transform->position.x;
                    int camTy = (int)cam->transform->position.y;
                    int camZone = GetZoneId(camTx, camTy);

                    if (camZone == zoneId)
                    {
                        terminal->controlledCameras.push_back(cam);
                    }
                }

                cctvTerminals.push_back(terminal);

                std::cout << "[FPSScene] Spawned terminal at (" << pos.x << ", " << pos.y
                          << ") in Zone " << zoneId << " controlling "
                          << terminal->controlledCameras.size() << " cameras\n";
            }

            // Register terminal positions with PlayerHUD for pathfinding
            for (size_t i = 0; i < terminalPositions.size(); ++i)
            {
                Vector2 pos = terminalPositions[i];
                int tileX = (int)std::floor(pos.x);
                int tileY = (int)std::floor(pos.y);
                int zoneId = GetZoneId(tileX, tileY);
                if (zoneId >= 1 && zoneId <= 4)
                {
                    playerHUD->SetTerminalPosition(zoneId, pos);
                }
            }

            // ===== Spawn Control Room Doors from map markers ('C' / tile 13) =====
            auto controlRoomDoorPositions = map->GetControlRoomDoorPositions();

            // Register control room door positions with PlayerHUD for pathfinding
            for (size_t i = 0; i < controlRoomDoorPositions.size(); ++i)
            {
                Vector2 pos = controlRoomDoorPositions[i];
                int tileX = (int)std::floor(pos.x);
                int tileY = (int)std::floor(pos.y);
                int zoneId = GetZoneId(tileX, tileY);
                if (zoneId >= 1 && zoneId <= 4)
                {
                    playerHUD->SetControlRoomPosition(zoneId, pos);
                }
            }

            for (size_t i = 0; i < controlRoomDoorPositions.size(); ++i)
            {
                Vector2 pos = controlRoomDoorPositions[i];

                // Determine zone from position
                int tileX = (int)std::floor(pos.x);
                int tileY = (int)std::floor(pos.y);
                int zoneId = GetZoneId(tileX, tileY);
                if (zoneId < 1 || zoneId > 4)
                {
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

            std::cout << "[FPSScene] ===== ACCESS CARD SPAWNING DEBUG =====\n";
            std::cout << "[FPSScene] Found " << accessCardPositions.size() << " access card markers in map\n";

            const char *cardSpritePaths[] = {
                "Assets/color_green_empty.png",
                "Assets/color_purple_empty.png",
                "Assets/color_red_empty.png",
                "Assets/color_yellow_empty.png"};

            // DEBUG: set true to force access cards to spawn at player spawn for debugging
            bool debugForceSpawnAtPlayer = true;

            for (size_t i = 0; i < accessCardPositions.size(); ++i)
            {
                Vector2 pos = accessCardPositions[i].first;
                int cardId = accessCardPositions[i].second;

                if (cardId < 0 || cardId > 3)
                    continue;

                const char *cardNames[] = {"Green", "Purple", "Red", "Yellow"};

                // If card marker lies within the player spawn exclusion area, find nearest valid tile
                auto findValidSpawn = [&](Engine::Vector2 orig) -> Engine::Vector2
                {
                    int ox = (int)std::floor(orig.x);
                    int oy = (int)std::floor(orig.y);
                    const int MAXR = 12;
                    for (int r = 0; r <= MAXR; ++r)
                    {
                        for (int dy = -r; dy <= r; ++dy)
                        {
                            for (int dx = -r; dx <= r; ++dx)
                            {
                                // only check perimeter of the square to keep it efficient
                                if (std::abs(dx) != r && std::abs(dy) != r)
                                    continue;
                                int nx = ox + dx;
                                int ny = oy + dy;
                                if (nx < 0 || nx >= mapWidth || ny < 0 || ny >= mapHeight)
                                    continue;
                                // skip tiles inside player spawn radius
                                float cdx = ((float)nx + 0.5f) - spawnExcludeX;
                                float cdy = ((float)ny + 0.5f) - spawnExcludeY;
                                if (cdx * cdx + cdy * cdy < spawnExcludeRadius * spawnExcludeRadius)
                                    continue;
                                // require walkable center of tile
                                if (!map->IsWalkable((float)nx + 0.5f, (float)ny + 0.5f))
                                    continue;
                                return Engine::Vector2((float)nx + 0.5f, (float)ny + 0.5f);
                            }
                        }
                    }
                    // fallback to original
                    return orig;
                };

                if (debugForceSpawnAtPlayer)
                {
                    // Spread forced-spawned cards around player spawn in a small circle
                    int total = (int)accessCardPositions.size();
                    if (total <= 0)
                        total = 1;
                    float angle = (2.0f * 3.14159265f) * ((float)i / (float)total);
                    float radius = 1.2f; // 1.2 tiles from spawn center
                    Engine::Vector2 candidate(spawnExcludeX + std::cos(angle) * radius,
                                              spawnExcludeY + std::sin(angle) * radius);
                    // if the candidate isn't walkable, fallback to nearest valid
                    if (map && !map->IsWalkable(candidate.x, candidate.y))
                    {
                        candidate = findValidSpawn(candidate);
                    }
                    pos = candidate;
                    std::cout << "[FPSScene][DEBUG] Forcing access card spawn near player spawn (" << pos.x << ", " << pos.y << ")\n";
                }
                else
                {
                    // if marker inside spawn radius, relocate
                    float ddx = pos.x - spawnExcludeX;
                    float ddy = pos.y - spawnExcludeY;
                    if (ddx * ddx + ddy * ddy < spawnExcludeRadius * spawnExcludeRadius)
                    {
                        std::cout << "[FPSScene] Access card at spawn area detected, searching for nearby valid spawn\n";
                        Engine::Vector2 newPos = findValidSpawn(pos);
                        pos = newPos;
                    }
                }

                std::cout << "[FPSScene] Spawning " << cardNames[cardId] << " card at (" << pos.x << ", " << pos.y << ")\n";

                // Create card GameObject
                std::string cardName = std::string(cardNames[cardId]) + "AccessCard";
                auto cardGO = CreateGameObject(cardName);
                cardGO->transform->position = Vector3(pos.x, pos.y, 0.5f);
                cardGO->transform->localScale = Vector2(0.4f, 0.4f);

                std::cout << "[FPSScene]   GameObject created: " << cardGO->name << "\n";

                // Add SpriteRenderer
                auto sr = cardGO->AddComponent<Engine::SpriteRenderer>();
                sr->SetSprite(Engine::AssetManager::GetSprite(cardSpritePaths[cardId]));
                sr->billboard = true;
                sr->sortingOrder = 200;

                std::cout << "[FPSScene]   SpriteRenderer added (billboard=" << sr->billboard << ", sortingOrder=" << sr->sortingOrder << ")\n";

                // Add Interactable
                auto interactable = cardGO->AddComponent<Engine::Interactable>();
                interactable->actionText = "Collect Access Card";
                interactable->maxDistance = 2.5f;

                std::cout << "[FPSScene]   Interactable added\n";

                // Add AccessCard component
                auto card = cardGO->AddComponent<Game::AccessCard>();
                card->cardId = cardId;
                card->cardColor = static_cast<Game::AccessCardColor>(cardId);
                card->spriteRenderer = sr;
                card->interactable = interactable;
                // Ensure card is active and visible after creation
                cardGO->active = true;
                std::cout << "[FPSScene]   AccessCard component added (cardId=" << cardId << ")\n";
                std::cout << "[FPSScene]   Card position: (" << cardGO->transform->position.x << ", "
                          << cardGO->transform->position.y << ", " << cardGO->transform->position.z << ")\n";
                if (!cardGO->active)
                {
                    std::cout << "[FPSScene][WARN] Card GameObject inactive after creation - forcing active=true\n";
                    cardGO->active = true;
                }
                std::cout << "[FPSScene]   Card active: " << (cardGO->active ? "YES" : "NO") << "\n";
            }

            std::cout << "[FPSScene] Access card system initialized: "
                      << controlRoomDoorPositions.size() << " doors, "
                      << accessCardPositions.size() << " cards\n";
            std::cout << "[FPSScene] ===== ACCESS CARD SPAWNING COMPLETE =====\n";

            // Connect terminals to guards
            for (auto *guard : guards)
            {
                if (guard)
                {
                    guard->SetTerminals(&cctvTerminals);
                }
            }

            // ===== Spawn NPCs: 2 Bully + 2 Annoying per zone (16 total) =====
            npcs.clear();
            for (int z = 1; z <= 4; ++z)
            {

                SpawnNPCInZone(z, NPCType::Bully, map, "Assets/bully.png");
                SpawnNPCInZone(z, NPCType::Annoying, map, "Assets/annoying_classmate.png");
            }
            std::cout << "[FPSScene] Spawned " << npcs.size() << " NPCs (4 per zone)\n";

            // Add terminal icons to minimap
            if (playerHUD && playerHUD->GetMinimap())
            {
                for (auto &pos : terminalPositions)
                {
                    playerHUD->GetMinimap()->AddTerminalIcon(
                        pos,
                        AssetManager::GetSprite("Assets/hill_top_smile.png"));
                }
            }

            /* CreateEndPanel();*/
            SwitchToPlayerCam();

            // Initialize Level Editor
            levelEditor.Initialize(this, map, playerCam, playerTf);

            PuzzleManager::BookDataInit();
            PuzzleManager::Init();

            Game::PowerUpManager::Init();

            // Create Bandit visualizer (hidden by default)
            {
                auto bvGO = CreateGameObject("BanditVisualizer");
                if (bvGO) {
                    banditVisualizer = bvGO->AddComponent<Game::BanditVisualizer>();
                    if (banditVisualizer) {
                        banditVisualizer->bandit = &bandit;
                        banditVisualizer->SetGuardsPointer(&guards);
                        // connect cctv scores + heatmap + zone grid so visualizer can display them
                        banditVisualizer->SetCCTVScorePointer(cctvScore);
                        banditVisualizer->SetHeatmapPointer(&heatmap);
                        banditVisualizer->SetZoneGridPointer(&zoneGrid);
                        banditVisualizer->SetSelectedZone(prevChosenZoneId);
                        banditVisualizer->SetVisible(false);
                    }
                }
            }
            // Load GameObjects from Editor JSON
            AEEditor::ObjectManager::LoadSceneFromJson("Assets/Config/Scenes/EditorScene.json");
        }

        void OnAfterRender() override
        {
            // Render level editor overlay on top of everything
            levelEditor.Render();
        }

        void OnUpdate() override
        {
            // Toggle Level Editor
            if (Input::GetKeyDown(KeyCode::F1))
            {
                levelEditor.ToggleEditor();
            }

            // Update editor (only if active)
            levelEditor.Update();

            // If editor is active, skip game logic
            if (levelEditor.IsActive())
            {
                return;
            }

            // Toggle ML debug summary (no flood)
            if (Input::GetKeyDown(KeyCode::M))
            {
                debugML = !debugML;
                mlSummaryTimer = 0.0f;
            }

            // Toggle bandit visualizer with F3
            if (Input::GetKeyDown(KeyCode::F3)) {
                if (banditVisualizer) {
                    banditVisualizer->ToggleVisibility();
                    // When showing visualizer, unlock cursor so user can interact
                    if (banditVisualizer) {
                        // panel visibility toggled inside ToggleVisibility(); check panel state indirectly
                        // We can't access m_panel here, so simply toggle cursor registration based on panel visible state
                        // Best-effort: register/unregister cursor using a simple toggle stored on banditVisualizer via visible state
                    }
                }
            }

            // CCTV view switching (silent)
            int camCount = (int)cctvViewPos.size();
            if (camCount > 0)
            {
                /* if (Input::GetKeyDown(KeyCode::DownArrow)) {
                     if (activeCCTV < 0) activeCCTV = 0;
                     else activeCCTV = (activeCCTV + 1) % camCount;
                     SwitchToCCTV(activeCCTV);
                 }
                 if (Input::GetKeyDown(KeyCode::UpArrow)) {
                     if (activeCCTV < 0) activeCCTV = camCount - 1;
                     else activeCCTV = (activeCCTV - 1 + camCount) % camCount;
                     SwitchToCCTV(activeCCTV);
                 }*/
            }
            if (Input::GetKeyDown(KeyCode::Alpha0))
            {
                SwitchToPlayerCam();
            }

            // Lose panel
            /*if (!endPanelShown && playerData) {
                if (playerData->IsCaught()) {
                    SoundManager::PlaySFX("lose");
                    ShowEndPanel("YOU GOT CAUGHT. GAME OVER", "Quit", []() {
                        SceneManager::LoadScene("FPSScene");
                        });
                }
            }*/

            if (!gameEnded && playerData && playerData->IsCaught())
            {
                gameEnded = true;
                if (loseScreen)
                {
                    loseScreen->Show();
                }
                return;
            }

            if (!playerData || !playerTf)
                return;
            if (playerData->GetState() != PlayerState::Normal)
                return;

            // Update game timer
            playerData->UpdateGameTime(Engine::Time::deltaTime);

            // Update HUD elements
            if (playerHUD)
            {
                // Check zone changes for entry notification
                int tx = (int)std::floor(playerTf->position.x);
                int ty = (int)std::floor(playerTf->position.y);
                int currentZone = GetZoneId(tx, ty);

                // Debug: Print player position and zone (only on zone change)
                static int lastPrintedZone = -1;
                if (currentZone != lastPrintedZone)
                {
                    std::cout << "[FPSScene] Player at (" << tx << ", " << ty << ") ? Zone " << currentZone
                              << " (" << Game::ZoneIdToLetter(currentZone) << ")"
                              << " | Map size: " << mapWidth << "x" << mapHeight
                              << " | Mid: " << (mapWidth / 2) << "," << (mapHeight / 2) << "\n";
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
                for (auto *guard : guards)
                {
                    if (!guard || !guard->transform)
                        continue;

                    Vector2 guardPos(guard->transform->position.x, guard->transform->position.y);
                    Vector2 playerPos(playerTf->position.x, playerTf->position.y);

                    float dx = guardPos.x - playerPos.x;
                    float dy = guardPos.y - playerPos.y;
                    float distance = std::sqrt(dx * dx + dy * dy);

                    if (distance < nearestGuardDistance)
                    {
                        nearestGuardDistance = distance;
                    }
                }

                // guard proximity handled by UpdateGuardProximityLoop (looped sound)

                float nearestCCTVDistance = 999999.0f;
                for (auto *cctv : cctvSensors)
                {
                    if (!cctv || !cctv->transform)
                        continue;

                    Vector2 cctvPos(cctv->transform->position.x, cctv->transform->position.y);
                    Vector2 playerPos(playerTf->position.x, playerTf->position.y);

                    float dx = cctvPos.x - playerPos.x;
                    float dy = cctvPos.y - playerPos.y;
                    float distance = std::sqrt(dx * dx + dy * dy);

                    if (distance < nearestCCTVDistance)
                    {
                        nearestCCTVDistance = distance;
                    }
                }

                float nearestTerminalDistance = 999999.0f;
                for (auto *terminal : cctvTerminals)
                {
                    if (!terminal || !terminal->transform)
                        continue;

                    Vector2 terminalPos(terminal->transform->position.x, terminal->transform->position.y);
                    Vector2 playerPos(playerTf->position.x, playerTf->position.y);

                    float dx = terminalPos.x - playerPos.x;
                    float dy = terminalPos.y - playerPos.y;
                    float distance = std::sqrt(dx * dx + dy * dy);

                    if (distance < nearestTerminalDistance)
                    {
                        nearestTerminalDistance = distance;
                    }
                }

                // NPC proximity: separate Bully vs Annoying
                float nearestAnnoyingDistance = 999999.0f;
                int annoyingCountNearby = 0;
                float nearestBullyDistance = 999999.0f;
                int bullyCountNearby = 0;
                const float npcNoticeRadius = 6.0f; // within this range we count them as 'nearby'
                for (auto *npc : npcs)
                {
                    if (!npc || !npc->transform)
                        continue;
                    Vector2 npcPos(npc->transform->position.x, npc->transform->position.y);
                    Vector2 playerPos(playerTf->position.x, playerTf->position.y);
                    float dx = npcPos.x - playerPos.x;
                    float dy = npcPos.y - playerPos.y;
                    float distance = std::sqrt(dx * dx + dy * dy);

                    if (npc->type == NPCType::Annoying)
                    {
                        if (distance < nearestAnnoyingDistance)
                            nearestAnnoyingDistance = distance;
                        if (distance <= npcNoticeRadius)
                            annoyingCountNearby++;
                    }
                    else if (npc->type == NPCType::Bully)
                    {
                        if (distance < nearestBullyDistance)
                            nearestBullyDistance = distance;
                        if (distance <= npcNoticeRadius)
                            bullyCountNearby++;
                    }
                }

                // Update guard proximity warning
                playerHUD->UpdateGuardProximity(nearestGuardDistance);
                // Update NPC approach indicators (Annoying + Bully)
                playerHUD->UpdateNPCApproach(annoyingCountNearby, nearestAnnoyingDistance, bullyCountNearby, nearestBullyDistance);
                SoundManager::UpdateGuardProximityLoop(nearestGuardDistance);
                SoundManager::UpdateCCTVHumLoop(nearestCCTVDistance);
                SoundManager::UpdateTerminalBuzzLoop(nearestTerminalDistance);

                // Apply camera shake based on guard proximity
                const float shakeDistance = 6.0f;
                if (nearestGuardDistance < shakeDistance && playerCam)
                {
                    auto cameraShake = playerCam->gameObject->GetComponent<CameraShake>();
                    if (!cameraShake)
                    {
                        cameraShake = playerCam->gameObject->AddComponent<CameraShake>();
                        cameraShake->Initialize();
                    }

                    // Intensity increases as guard gets closer
                    float intensity = (1.0f - (nearestGuardDistance / shakeDistance)) * 0.08f;
                    cameraShake->Shake(0.1f, intensity);
                }

                // Update alert level display
                int guardCount = 0;
                for (auto *g : guards)
                {
                    if (g && (g->IsInChaseState() || g->IsInSearchState()))
                    {
                        guardCount++;
                    }
                }

                // Determine alert stage (1-4 based on escalation)
                int alertStage = 1;
                if (fullEscalationActive)
                    alertStage = 4;
                else if (priorityZoneHotTimer > fullEscalationThreshold * 0.66f)
                    alertStage = 3;
                else if (priorityZoneHotTimer > fullEscalationThreshold * 0.33f)
                    alertStage = 2;
                // Play full escalation SFX immediately when escalation triggers
                static bool escalationSfxPlayed = false;
                if (fullEscalationActive && !escalationSfxPlayed)
                {
                    SoundManager::PlaySFX("full_escalation");
                    escalationSfxPlayed = true;
                }
                if (!fullEscalationActive)
                    escalationSfxPlayed = false;

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
                auto *s = cctvSensors[i];
                if (!s)
                    continue;

                if (cctvCooldown[i] > 0.0f)
                    cctvCooldown[i] -= Engine::Time::deltaTime;

                if (s->hasLastSeen)
                {
                    int tx = (int)s->lastSeenWorld.x;
                    int ty = (int)s->lastSeenWorld.y;
                    int zid = GetZoneId(tx, ty);

                    if (zid >= 1 && zid <= 4) {

                        // ALWAYS show in visualizer (real-time log)
                        if (banditVisualizer) {
                            banditVisualizer->SetCCTVStatus(
                                zid,
                                s->lastSeenWorld.x,
                                s->lastSeenWorld.y,
                                true
                            );
                        }

                        // ONLY spike score if cooldown allows
                        if (cctvCooldown[i] <= 0.0f) {
                            cctvScore[zid] += cctvSpikeAmount;

                            priorityZoneId = zid;
                            ApplyPriorityZone_AssistNearest(zid);

                            for (auto* g : guards)
                                if (g) g->OnCCTVSpotted(s->lastSeenWorld);

                            cctvCooldown[i] = cctvSpikeCooldown;
                        }
                    }
                }

                // clear every frame so it doesn't spam
                s->hasLastSeen = false;
            }

            // Bandit tick
            banditTimer += Engine::Time::deltaTime;
            if (banditTimer >= banditInterval)
            {
                banditTimer = 0.0f;
                BanditTick();
            }

            // Update visualizer selected zone and summary when bandit changes
            if (banditVisualizer) {
                banditVisualizer->SetSelectedZone(prevChosenZoneId);
            }

            // ML summary every 5 sec only when debug on
            if (debugML)
            {
                mlSummaryTimer += Engine::Time::deltaTime;
                if (mlSummaryTimer >= 5.0f)
                {
                    mlSummaryTimer = 0.0f;
                    PrintMLSummary();
                }
            }

            // Win check
            Vector2 p(playerTf->position.x, playerTf->position.y);
            float dx = exitPos.x - p.x;
            float dy = exitPos.y - p.y;

            if (!printedWin && (dx * dx + dy * dy) <= winRadius * winRadius)
            {
                printedWin = true;
                gameEnded = true;
                SoundManager::PlaySFX("win");

                // Update high score if this is a new record
                float currentTime = playerData->GetGameTime();
                if (currentTime < playerData->GetHighScore())
                {
                    playerData->SetHighScore(currentTime);
                    std::cout << "NEW HIGH SCORE: " << (int)(currentTime / 60) << ":"
                              << std::setfill('0') << std::setw(2) << ((int)currentTime % 60) << "!\n";
                }

                playerData->Win();
                std::cout << "YOU ESCAPED. WIN!\n";
                if (winScreen)
                {
                    winScreen->Show();
                }
            }
        }
        void OnUnload() override
        {
            // Disconnect guards from terminal list to avoid dangling pointers
            for (auto *g : guards)
            {
                if (g)
                    g->SetTerminals(nullptr);
            }

            // Clear terminals: remove references to cameras, HUD and guards
            for (auto *t : cctvTerminals)
            {
                if (!t)
                    continue;
                t->controlledCameras.clear();
                t->playerHUD = nullptr;
                t->spriteRenderer = nullptr;
                t->allGuards.clear();
                t->floatingIcon = nullptr;
            }

            // Clear sensors: null out external pointers so Update() won't dereference them
            for (auto *s : cctvSensors)
            {
                if (!s)
                    continue;
                s->tf = nullptr;
                s->tile = nullptr;
                s->hud = nullptr;
                s->minimap = nullptr;
                s->spriteRenderer = nullptr;
            }

            // Clear all scene-owned lists to avoid keeping stale pointers across reload
            cctvSensors.clear();
            cctvTerminals.clear();
            cctvViewPos.clear();
            cctvViewRot.clear();
            cctvCooldown.clear();

            guards.clear();

            // Clear other references
            powerUps.clear();
            powerUpBaseZ.clear();

            playerHUD = nullptr;
            playerData = nullptr;
            playerController = nullptr;
            playerTf = nullptr;
            playerCam = nullptr;

            PuzzleManager::FreeManager();


            loseScreen = nullptr;
            winScreen = nullptr;

            /*if (modalCanvas) {
                UI::UIManager::GetInstance()->DestroyCanvas(modalCanvas);
                modalCanvas = nullptr;
            }
            */

            // Clear bandit visualizer reference
            banditVisualizer = nullptr;

            AEEditor::ObjectManager::loadedObjects.clear();
            AEEditor::ObjectManager::selectedObject = nullptr;
            AEEditor::ObjectManager::draggingObject = nullptr;
        }
    };

} // namespace Game