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
// #include "Game/LevelEditor/LevelEditor.hpp"
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
        std::vector<Game::Door *> doors;
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

        // LevelEditor levelEditor;
        Game::BanditVisualizer *banditVisualizer = nullptr;

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
        }

        void SwitchToPlayerCam()
        {
            activeCCTV = -1;
            if (!playerTf)
                return;
            playerTf->position = playerViewPos;
            playerTf->rotation = playerViewRot;
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
            if (switchCooldown < 0.0f)
                switchCooldown = 0.0f;

            int nextArm = bandit.SelectArm();
            if (nextArm < 0)
                return;

            int nextZone = nextArm + 1;

            if (switchCooldown > 0.0f)
            {
                nextZone = priorityZoneId;
                nextArm = nextZone - 1;
            }
            else
            {
                if (nextZone != priorityZoneId)
                {
                    if (cctvScore[nextZone] > banditSwitchStrongEvidenceThreshold)
                    {
                        switchCooldown = banditSwitchCooldownStrongSec;
                    }
                    else
                    {
                        switchCooldown = banditSwitchCooldownNormalSec;
                    }
                }
            }

            bandit.Update(nextArm, zoneReward[nextZone]);

            if (nextZone == priorityZoneId)
            {
                priorityZoneHotTimer += banditInterval;
            }
            else
            {
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
