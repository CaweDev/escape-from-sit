// =======================
// SecurityGuard.hpp
// =======================
#pragma once

#include "Engine/Core/Component.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/TileMap/Door.hpp"

#include "Game/Player/Player.hpp"
#include <vector>
#include <queue>
#include <unordered_map>

#include "Game/UCB1Bandit/UCB1Bandit.hpp"
#include "Game/Heatmap/Heatmap.hpp"
#include "Game/ZoneManager/ZoneManager.hpp"



namespace Game {

    class CCTVTerminal;

    class SecurityGuard : public Engine::Component {
    public:
        Engine::TileMap* tileMap = nullptr;
        Engine::Transform* playerTf = nullptr;
        Player* playerData = nullptr;

        Game::ZoneManager* zoneMgr = nullptr;
        Game::Heatmap* heatmap = nullptr;
        std::vector<CCTVTerminal*>* cctvTerminals = nullptr;

        float patrolSpeed = 1.5f;
        float chaseSpeed = 2.4f;
        float searchSpeed = 0.8f;

        float arriveDistance = 0.10f;
        float detectRadius = 4.0f;
        float catchRadius = 0.7f;

        float maxLostSightTime = 2.5f;
        float searchDuration = 4.0f;

        void Initialize() override;
        void Update() override;
        void UpdateStateColor();

        void OnCCTVSpotted(const Engine::Vector2& worldPos);
        void OnCCTVSystemSabotaged(const Engine::Vector2& controlRoomPos, [[maybe_unused]] int sabotagedZoneId);
        void SetTerminals(std::vector<CCTVTerminal*>* terminals) { cctvTerminals = terminals; }

        // ML Zone Control
        void SetPatrolZoneOverride(int zoneId);
        int  GetPatrolZoneOverride() const { return m_hasForcedZone ? m_forcedZoneId : -1; }

        int homeZoneId = 1;

        bool IsInPatrolState() const { return state == GuardState::Patrol; }
        bool IsInChaseState() const { return state == GuardState::Chase; }
        bool IsInSearchState() const { return state == GuardState::Search; }

        void SetAssistZone(int zoneId);
        void ClearAssist();
        void SetFullEscalation(int zoneId);
        void ClearFullEscalation();

        Engine::Transform* transform = nullptr;

        // CCTV sabotage response (public for ML system access)
        bool investigatingSabotage = false;

    private:
        enum class GuardState { Patrol = 0, Chase, Search, Catch };
        GuardState state = GuardState::Patrol;

        bool assisting = false;
        bool fullEscalation = false;
        
        // Sabotage tracking (moved from public section)
        Engine::Vector2 sabotageLocation{};
        float sabotageInvestigateTimer = 0.0f;

        bool m_hasForcedZone = false;
        int  m_forcedZoneId = 1;

        float zoneCommitTimer = 0.0f;
        float zoneCommitDuration = 3.0f;

        float cctvPursuitTimer = 0.0f;
        float cctvPursuitDuration = 4.0f;

        struct GuardMemory {
            Engine::Vector2 lastSeenPos{};
            bool hasLastSeen = false;
            float timeSinceSeen = 0.0f;
        } memory;

        std::vector<Engine::Vector2> path;
        int pathIndex = 0;

        float repathTimer = 0.0f;
        float searchTimer = 0.0f;

        Engine::Door* heldDoor = nullptr;
        float doorHoldReleaseDist = 1.25f;

        std::unordered_map<int, int> visitCount;
        int lastTileId = -1;

        int patrolSampleCount = 40;
        float patrolMinGoalDist = 6.0f;

        float learnRepathTimer = 0.0f;
        float chooseInterval = 2.0f;

        static bool seeded;

    private:
        void SetState(GuardState newState);
        const char* StateToString(GuardState s) const;

        void PatrolUpdate();
        void ChaseUpdate();
        void SearchUpdate();

        bool CanSeePlayer() const;
        bool IsCaught() const;

        void FollowPath(float speed);
        void UpdateDoorAuto();
        Engine::Door* FindDoorAtTile(int tx, int ty) const;

        void BuildPathBFS_ToGoal(int gx, int gy);
        void BuildPathBFS_PatrolExplore();

        int iD(int x, int y) const { return y * tileMap->width + x; }
        Engine::Vector2 WorldFromTile(int x, int y) const { return Engine::Vector2((float)x + 0.5f, (float)y + 0.5f); }
        void GetCurrentTile(int& tx, int& ty) const;
        float DistSqTo(float x, float y) const;
    };

} // namespace Game