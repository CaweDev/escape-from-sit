#pragma once

#include "Engine/Core/Component.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/TileMap/Door.hpp"
#include "Player.hpp"

#include <vector>
#include <queue>
#include <unordered_map>

namespace Game {

    class SecurityGuard : public Engine::Component {
    public:
        Engine::TileMap* tileMap = nullptr;
        Engine::Transform* playerTf = nullptr;
        Player* playerData = nullptr;

        float patrolSpeed = 1.5f;
        float chaseSpeed = 2.4f;
        float searchSpeed = 0.8f;

        float arriveDistance = 0.10f;
        float detectRadius = 4.0f;
        float catchRadius = 0.7f;

        // Search tuning
        float maxLostSightTime = 2.5f;
        float searchDuration = 4.0f;

        void Initialize() override;
        void Update() override;
        void UpdateStateColor();

        void OnCCTVSpotted(const Engine::Vector2& worldPos);

    private:
        enum class GuardState { Patrol = 0, Chase, Search, Catch };
        GuardState state = GuardState::Patrol;

        struct GuardMemory {
            Engine::Vector2 lastSeenPos{};
            bool hasLastSeen = false;
            float timeSinceSeen = 0.0f;
        } memory;

        // Path
        std::vector<Engine::Vector2> path;
        int pathIndex = 0;

        // Timers
        float repathTimer = 0.0f;
        float searchTimer = 0.0f;

        // Door auto-open
        Engine::Door* heldDoor = nullptr;
        float doorHoldReleaseDist = 1.25f;

        // Patrol exploration memory (tile visits)
        std::unordered_map<int, int> visitCount;
        int lastTileId = -1;

        // Patrol goal selection tuning
        int patrolSampleCount = 40;     // how many random tiles to sample
        float patrolMinGoalDist = 6.0f; // avoid picking goals too close

        static bool seeded;

    private:
        // FSM helpers
        void SetState(GuardState newState);
        const char* StateToString(GuardState s) const;

        // Updates
        void PatrolUpdate();
        void ChaseUpdate();
        void SearchUpdate();

        // Perception
        bool CanSeePlayer() const;
        bool IsCaught() const;

        // Movement + doors
        void FollowPath(float speed);
        void UpdateDoorAuto();
        Engine::Door* FindDoorAtTile(int tx, int ty) const;

        // BFS pathing
        void BuildPathBFS_ToGoal(int gx, int gy);

        // NEW: smarter patrol goal selection
        void BuildPathBFS_PatrolExplore();

        // Tile helpers
        int iD(int x, int y) const { return y * tileMap->width + x; }
        Engine::Vector2 WorldFromTile(int x, int y) const { return Engine::Vector2((float)x + 0.5f, (float)y + 0.5f); }
        void GetCurrentTile(int& tx, int& ty) const;
        float DistSqTo(float x, float y) const;
    };

} // namespace Game
