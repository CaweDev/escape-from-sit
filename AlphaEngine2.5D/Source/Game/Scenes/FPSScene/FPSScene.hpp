#pragma once
/*!
 * \file FPSScene.hpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief Main FPS gameplay scene declaration and helpers
 */

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
#include "Game/LevelEditor/LevelEditor.hpp"
#include "Game/AccessCard/AccessCard.hpp"
#include "Game/ControlRoomDoor/ControlRoomDoor.hpp"
#include "Game/PathRenderer/PathRenderer.hpp"
#include "Game/PowerUp/PowerUpManager.hpp"
#include "Game/Player/PlayerPowerUpController.hpp"
#include "Game/Inventory/Inventory.hpp"

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

    /*! \class FPSScene
     *  \brief Core gameplay scene for FPS-style gameplay and AI systems
     */
    class FPSScene : public Scene
    {
    public:
        FPSScene() : Scene("FPSScene") {}

        bool gameEnded = false;

        FPSController *playerController = nullptr;
        Player *playerData = nullptr;
        Transform *playerTf = nullptr;
        Camera *playerCam = nullptr;
        PlayerHUD *playerHUD = nullptr;
        PlayerPowerUpController *powerUpController = nullptr;

        std::vector<Game::SecurityGuard *> guards;
        std::vector<Game::CCTVCamera *> cctvSensors;
        std::vector<Game::CCTVTerminal *> cctvTerminals;

        int activeCCTV = -1;

        Vector3 playerViewPos{};
        float playerViewRot = 0.0f;

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
                    return z.id;
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

        Game::ZoneManager zoneMgr;
        Game::Heatmap heatmap;
        Game::UCB1Bandit bandit{4, 2.0f};

        std::vector<int> zoneGrid;

        float banditTimer = 0.0f;
        float banditInterval = 2.0f;
        int prevChosenZoneId = 1;

        float wHeat = 0.55f;
        float wCCTV = 0.45f;
        float heatRef = 50.0f;
        float cctvRef = 6.0f;

        float heatAddPerSec = 1.0f;
        float heatDecayPerSec = 0.985f;

        float banditSwitchStrongEvidenceThreshold = 0.5f;
        float banditSwitchCooldownStrongSec = 25.0f;
        float banditSwitchCooldownNormalSec = 8.0f;
        float cctvScoreDecayFactor = 0.85f;

        float cctvScore[5] = {0, 0, 0, 0, 0};
        std::vector<float> cctvCooldown;
        float cctvSpikeCooldown = 1.0f;
        float cctvSpikeAmount = 1.0f;

        int priorityZoneId = 1;

        float priorityZoneHotTimer = 0.0f;
        float fullEscalationThreshold = 30.0f;
        bool fullEscalationActive = false;

        bool debugML = false;
        float mlSummaryTimer = 0.0f;

        std::vector<Game::NPC *> npcs;

        bool m_annoyingLoopPlaying = false;
        bool m_annoyingSfxTriggered = false;
        float m_annoyingLoopLastDistance = 999999.0f;

        LevelEditor levelEditor;
        Game::BanditVisualizer *banditVisualizer = nullptr;

        int mapWidth = 0;
        int mapHeight = 0;

    private:
        float spawnExcludeX = 2.5f;
        float spawnExcludeY = 2.5f;
        float spawnExcludeRadius = 7.0f;

        int spawnPickAttempts = 100;
        int spawnFallbackMaxRadius = 20;

        float bullyNPCSpeed = 2.0f;
        float bullyNPCTriggerRadius = 3.0f;
        float annoyingNPCSpeed = 1.2f;
        float annoyingNPCTriggerRadius = 2.5f;
        float annoyingNPCBlockTime = 1.5f;

        float spawnedNPCColliderRadius = 0.25f;
        float spawnedGuardColliderRadius = 0.3f;

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
        }

        void SwitchToPlayerCam()
        {
            activeCCTV = -1;
            if (!playerTf)
                return;
            playerTf->position = playerViewPos;
            playerTf->rotation = playerViewRot;
        }


        Game::NPC *SpawnNPCInZone(int zoneId, NPCType npcType, TileMap *map, const char *spritePath)
        {
            int gx = 0, gy = 0;
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
            for (int attempt = 0; attempt < spawnPickAttempts; ++attempt)
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
                Vector2 c = GetZoneCenter(zoneId);
                int cx = (int)std::floor(c.x);
                int cy = (int)std::floor(c.y);
                const int MAXR = spawnFallbackMaxRadius;
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
                            gx = nx;
                            gy = ny;
                            placed = true;
                            break;
                        }
                    }
                }
                if (!placed)
                {
                    bool foundAny = false;
                    Vector2 zoneCenter = GetZoneCenter(zoneId);
                    int zx = (int)std::floor(zoneCenter.x);
                    int zy = (int)std::floor(zoneCenter.y);
                    int SCAN_MAX = (map->width > map->height) ? map->width : map->height;
                    for (int r = 0; r <= SCAN_MAX && !foundAny; ++r) {
                        for (int dy = -r; dy <= r && !foundAny; ++dy) {
                            for (int dx = -r; dx <= r && !foundAny; ++dx) {
                                if (std::abs(dx) != r && std::abs(dy) != r) continue;
                                int nx = zx + dx;
                                int ny = zy + dy;
                                if (nx < 0 || nx >= map->width || ny < 0 || ny >= map->height) continue;
                                float sdx = (float)nx + 0.5f - spawnExcludeX;
                                float sdy = (float)ny + 0.5f - spawnExcludeY;
                                if (sdx * sdx + sdy * sdy < spawnExcludeRadius * spawnExcludeRadius) continue;
                                if (!map->IsWalkable((float)nx + 0.5f, (float)ny + 0.5f)) continue;
                                if (IsDoorTile(nx, ny)) continue;
                                gx = nx;
                                gy = ny;
                                foundAny = true;
                                break;
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

            npc->SetPickGoalInZone([this](int zId, int &outX, int &outY) -> bool
                                   { return zoneMgr.PickRandomWalkableInZone(zId, outX, outY); });

            if (npcType == NPCType::Bully)
            {
                npc->speed = bullyNPCSpeed;
                npc->triggerRadius = bullyNPCTriggerRadius;
            }
            else
            {
                npc->speed = annoyingNPCSpeed;
                npc->triggerRadius = annoyingNPCTriggerRadius;
                npc->blockTime = annoyingNPCBlockTime;
            }

            auto npcRb = go->AddComponent<Rigidbody2D>();
            npcRb->bodyType = RigidbodyType2D::Dynamic;
            npcRb->gravityScale = 0.0f;
            npcRb->drag = 0.0f;

            auto npcCol = go->AddComponent<CircleCollider2D>();
            npcCol->radius = spawnedNPCColliderRadius;
            npcCol->minHeight = 0.0f;
            npcCol->maxHeight = 2.0f;

            auto sr = go->AddComponent<SpriteSheetRenderer>();
            sr->SetSpriteSheet(AssetManager::GetSpriteSheet(spritePath));
            sr->billboard = true;
            sr->sortingOrder = 90;
            sr->Play("Patrol");

            npcs.push_back(npc);

            return npc;
        }


        void ApplyPriorityZone_AssistNearest(int pZone)
        {
            priorityZoneId = pZone;

            bool anyGuardEngaged = false;
            for (auto *g : guards)
            {
                if (!g)
                    continue;
                if (g->homeZoneId == pZone || g->GetPatrolZoneOverride() == pZone)
                {
                    if (g->investigatingSabotage)
                        continue;

                    if (g->IsInChaseState() || g->IsInSearchState())
                    {
                        anyGuardEngaged = true;
                        break;
                    }
                }
            }

            if (anyGuardEngaged)
            {
                priorityZoneHotTimer = fullEscalationThreshold;
            }

            if (priorityZoneHotTimer >= fullEscalationThreshold && !fullEscalationActive)
            {
                fullEscalationActive = true;

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
            float zoneReward[5] = {0, 0, 0, 0, 0};

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

            if (switchCooldown > 0.0f) {
                nextZone = priorityZoneId;
                nextArm = nextZone - 1;
            }
            else {
                if (nextZone != priorityZoneId) {
                    if (cctvScore[nextZone] > banditSwitchStrongEvidenceThreshold) {
                        switchCooldown = banditSwitchCooldownStrongSec;
                    }
                    else {
                        switchCooldown = banditSwitchCooldownNormalSec;
                    }
                }
            }

            bandit.Update(nextArm, zoneReward[nextZone]);

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

            for (int z = 1; z <= 4; ++z)
                cctvScore[z] *= cctvScoreDecayFactor;
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
