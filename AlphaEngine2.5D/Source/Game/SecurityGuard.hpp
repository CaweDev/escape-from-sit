#pragma once
#include "Engine/Core/Component.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include "Engine/TileMap/Door.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Core/Time.hpp"
#include <Engine/Graphics/SpriteRenderer.hpp>

#include "Player.hpp"
#include <vector>
#include <queue>

namespace Game {

    class SecurityGuard : public Engine::Component {
    public:
        Engine::TileMap* tileMap = nullptr;
        Engine::Transform* playerTf = nullptr;
        Player* playerData = nullptr;

        float patrolSpeed = 1.5f;
        float chaseSpeed = 2.4f;
        float arriveDistance = 0.10f;

        float detectRadius = 4.0f;
        float catchRadius = 0.7f;

        void Initialize() override;
        void Update() override;

        void OnCCTVSpotted(const Engine::Vector2& worldPos);

    private:
        enum class GuardState { Patrol = 0, Chase, Search, Catch };
        GuardState state = GuardState::Patrol;

        // Path
        std::vector<Engine::Vector2> path;
        int pathIndex = 0;

        // RNG seed
        static bool seeded;

        // Memory (replaces lastSeenPos + lostSightTimer + hasLastSeen)
        struct GuardMemory {
            Engine::Vector2 lastSeenPos{};
            float timeSinceSeen = 0.0f;
            bool hasLastSeen = false;
        } memory;

        float maxLostSightTime = 2.5f;
        float repathTimer = 0.0f;

        // Search behaviour
        float searchTimer = 0.0f;
        float searchDuration = 3.0f;
        float searchSpeed = 0.8f; 
        float searchRepathTimer = 0.0f;

        // Door handling
        Engine::Door* heldDoor = nullptr;
        float doorHoldReleaseDist = 1.2f;

    private:
        // FSM updates
        void PatrolUpdate();
        void ChaseUpdate();
        void SearchUpdate();

        // FSM logging helper
        const char* StateToString(GuardState s) const;
        void SetState(GuardState newState);

        // Visual debug
        Engine::Color GetColorForState() const;
        void ApplyStateColor();

        // Perception
        bool CanSeePlayer() const;
        bool IsCaught() const;

        // Movement
        void FollowPath(float speed);

        // BFS
        void BuildPathBFS_Random();
        void BuildPathBFS_ToGoal(int gx, int gy);

        // Tile helpers
        int iD(int x, int y) const { return y * tileMap->width + x; }
        Engine::Vector2 WorldFromTile(int x, int y) const {
            return Engine::Vector2((float)x + 0.5f, (float)y + 0.5f);
        }

        // Doors
        Engine::Door* FindDoorAtTile(int tx, int ty) const;
        void UpdateDoorAuto();
    };

} // namespace Game