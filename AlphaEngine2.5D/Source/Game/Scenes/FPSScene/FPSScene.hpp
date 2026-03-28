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
#include "Game/Scenes/MainMenuScene/UI/ControlsMenu.hpp"
#include "Game/Puzzle/PuzzleManager.hpp"
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
#include "Game/PowerUp/PowerUpManager.hpp"
#include "Game/Player/PlayerPowerUpController.hpp"

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
        Game::ControlsMenu *controlsMenu = nullptr;
        bool showControlsAtStart = false;

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
        Game::ZoneManager zoneMgr;
        Game::Heatmap     heatmap;
        Game::UCB1Bandit  bandit{ 4, 2.0f };

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

        // Annoying-classmate loop channel key
        bool m_annoyingLoopPlaying = false;
        // One-shot SFX trigger guard for very-close annoying NPCs
        bool m_annoyingSfxTriggered = false;
        float m_annoyingLoopLastDistance = 999999.0f;

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
        static constexpr float spawnExcludeRadius = 7.0f; // increase to keep guards/NPCs farther from player spawn

        Game::NPC *SpawnNPCInZone(int zoneId, NPCType npcType, TileMap *map, const char *spritePath)
        {
            int gx = 0, gy = 0;
            // helper: check if a tile coordinate corresponds to a door/exit/control-room door
            auto IsDoorTile = [&](int tx, int ty) {
                if (!map) return false;
                for (auto &p : map->GetSpawnedDoorPositionsPublic()) {
                    if ((int)std::floor(p.x) == tx && (int)std::floor(p.y) == ty) return true;
                }
                for (auto &p : map->GetControlRoomDoorPositions()) {
                    if ((int)std::floor(p.x) == tx && (int)std::floor(p.y) == ty) return true;
                }
                for (auto &p : map->GetExitPositions()) {
                    if ((int)std::floor(p.x) == tx && (int)std::floor(p.y) == ty) return true;
                }
                return false;
            };

            bool found = false;
            // Try multiple times to find a position away from the player spawn area and not on doors
            for (int attempt = 0; attempt < 100; ++attempt)
            {
                if (!zoneMgr.PickRandomWalkableInZone(zoneId, gx, gy))
                    break;
                float dx = (float)gx + 0.5f - spawnExcludeX;
                float dy = (float)gy + 0.5f - spawnExcludeY;
                if (dx * dx + dy * dy <= spawnExcludeRadius * spawnExcludeRadius) continue;
                if (IsDoorTile(gx, gy)) continue;
                found = true;
                break;
            }

            if (!found)
            {
                // fallback: search outward from zone center for a valid tile
                Vector2 c = GetZoneCenter(zoneId);
                int cx = (int)std::floor(c.x);
                int cy = (int)std::floor(c.y);
                const int MAXR = 20;
                bool placed = false;
                for (int r = 0; r <= MAXR && !placed; ++r) {
                    for (int dy = -r; dy <= r && !placed; ++dy) {
                        for (int dx = -r; dx <= r && !placed; ++dx) {
                            if (std::abs(dx) != r && std::abs(dy) != r) continue;
                            int nx = cx + dx;
                            int ny = cy + dy;
                            if (nx < 0 || nx >= map->width || ny < 0 || ny >= map->height) continue;
                            float sdx = (float)nx + 0.5f - spawnExcludeX;
                            float sdy = (float)ny + 0.5f - spawnExcludeY;
                            if (sdx * sdx + sdy * sdy < spawnExcludeRadius * spawnExcludeRadius) continue;
                            if (!map->IsWalkable((float)nx + 0.5f, (float)ny + 0.5f)) continue;
                            if (IsDoorTile(nx, ny)) continue;
                            gx = nx; gy = ny; placed = true; break;
                        }
                    }
                }
                if (!placed)
                {
                    bool foundAny = false;
                    // Try scanning outward from the zone center
                    Vector2 zoneCenter = GetZoneCenter(zoneId);
                    int zx = (int)std::floor(zoneCenter.x);
                    int zy = (int)std::floor(zoneCenter.y);
                    // Expand outward from center of zone
                    int SCAN_MAX = (map->width > map->height) ? map->width : map->height;
                    for (int r = 0; r <= SCAN_MAX && !foundAny; ++r) {
                        for (int dy = -r; dy <= r && !foundAny; ++dy) {
                            for (int dx = -r; dx <= r && !foundAny; ++dx) {
                                if (std::abs(dx) != r && std::abs(dy) != r) continue;
                                int nx = zx + dx;
                                int ny = zy + dy;
                                if (nx < 0 || nx >= map->width || ny < 0 || ny >= map->height) continue;
                                // skip spawn area
                                float sdx = (float)nx + 0.5f - spawnExcludeX;
                                float sdy = (float)ny + 0.5f - spawnExcludeY;
                                if (sdx * sdx + sdy * sdy < spawnExcludeRadius * spawnExcludeRadius) continue;
                                if (!map->IsWalkable((float)nx + 0.5f, (float)ny + 0.5f)) continue;
                                // also skip tiles that are doors or exits
                                if (IsDoorTile(nx, ny)) continue;
                                gx = nx; gy = ny; foundAny = true; break;
                            }
                        }
                    }
                    if (!foundAny) {
                        Vector2 c2 = GetZoneCenter(zoneId);
                        gx = (int)c2.x;
                        gy = (int)c2.y;
                    }
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
            // helper: check if a tile coordinate corresponds to a door/exit/control-room door
            auto IsDoorTile = [&](int tx, int ty) {
                if (!map) return false;
                for (auto &p : map->GetSpawnedDoorPositionsPublic()) {
                    if ((int)std::floor(p.x) == tx && (int)std::floor(p.y) == ty) return true;
                }
                for (auto &p : map->GetControlRoomDoorPositions()) {
                    if ((int)std::floor(p.x) == tx && (int)std::floor(p.y) == ty) return true;
                }
                for (auto &p : map->GetExitPositions()) {
                    if ((int)std::floor(p.x) == tx && (int)std::floor(p.y) == ty) return true;
                }
                return false;
            };

            bool found = false;
            // Try multiple times to find a position that is walkable, not inside player spawn area and not a door
            for (int attempt = 0; attempt < 100; ++attempt) {
                if (!zoneMgr.PickRandomWalkableInZone(zoneId, gx, gy)) break;
                // exclude player spawn area
                float dx = (float)gx + 0.5f - spawnExcludeX;
                float dy = (float)gy + 0.5f - spawnExcludeY;
                if (dx * dx + dy * dy < spawnExcludeRadius * spawnExcludeRadius) continue;
                // exclude door tiles
                if (IsDoorTile(gx, gy)) continue;
                found = true;
                break;
            }

            if (!found) {
                // fallback: try zone center then search outward for a valid tile
                Vector2 c = GetZoneCenter(zoneId);
                int cx = (int)std::floor(c.x);
                int cy = (int)std::floor(c.y);
                const int MAXR = 20;
                bool placed = false;
                for (int r = 0; r <= MAXR && !placed; ++r) {
                    for (int dy = -r; dy <= r && !placed; ++dy) {
                        for (int dx = -r; dx <= r && !placed; ++dx) {
                            // check perimeter only for efficiency
                            if (std::abs(dx) != r && std::abs(dy) != r) continue;
                            int nx = cx + dx;
                            int ny = cy + dy;
                            if (nx < 0 || nx >= map->width || ny < 0 || ny >= map->height) continue;
                            // skip spawn area
                            float sdx = (float)nx + 0.5f - spawnExcludeX;
                            float sdy = (float)ny + 0.5f - spawnExcludeY;
                            if (sdx * sdx + sdy * sdy < spawnExcludeRadius * spawnExcludeRadius) continue;
                            // must be walkable and not a door
                            if (!map->IsWalkable((float)nx + 0.5f, (float)ny + 0.5f)) continue;
                            if (IsDoorTile(nx, ny)) continue;
                            gx = nx; gy = ny; placed = true; break;
                        }
                    }
                }
                if (!placed) {
                    bool foundAny = false;
                    int zx2 = cx; int zy2 = cy;
                    int SCAN_MAX2 = (map->width > map->height) ? map->width : map->height;
                    for (int r2 = 0; r2 <= SCAN_MAX2 && !foundAny; ++r2) {
                        for (int dy2 = -r2; dy2 <= r2 && !foundAny; ++dy2) {
                            for (int dx2 = -r2; dx2 <= r2 && !foundAny; ++dx2) {
                                if (std::abs(dx2) != r2 && std::abs(dy2) != r2) continue;
                                int nx = zx2 + dx2;
                                int ny = zy2 + dy2;
                                if (nx < 0 || nx >= map->width || ny < 0 || ny >= map->height) continue;
                                float sdx = (float)nx + 0.5f - spawnExcludeX;
                                float sdy = (float)ny + 0.5f - spawnExcludeY;
                                if (sdx * sdx + sdy * sdy < spawnExcludeRadius * spawnExcludeRadius) continue;
                                if (!map->IsWalkable((float)nx + 0.5f, (float)ny + 0.5f)) continue;
                                if (IsDoorTile(nx, ny)) continue;
                                gx = nx; gy = ny; foundAny = true; break;
                            }
                        }
                    }
                    if (!foundAny) {
                        // last resort fallback to center
                        gx = cx; gy = cy;
                    }
                }
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
        void OnLoad() override;
        void OnAfterRender() override;
        void OnUpdate() override;
        void OnUnload() override;
    };

} // namespace Game

#ifndef FPS_SCENE_CPP_COMPILATION_UNIT
#include "Game/Scenes/FPSScene/FPSScene.cpp"
#endif
