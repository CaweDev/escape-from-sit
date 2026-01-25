#pragma once
#include "Engine/Core/Component.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/TileMap/TileMap.hpp"
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

        Engine::Vector2 cctvLastSeenPos{};
        bool hasCctvLastSeen = false;

        void SetCCTVLastSeen(Engine::Vector2 worldPos) {
            cctvLastSeenPos = worldPos;
            hasCctvLastSeen = true;
        }

    private:
        enum class GuardState {
            Patrol = 0,
            Chase,
            Catch
        };

        GuardState state = GuardState::Patrol;

        std::vector<Engine::Vector2> path;
        int pathIndex = 0;

        static bool seeded;

        Engine::Vector2 lastSeenPos{};
        bool hasLastSeen = false;

        float lostSightTimer = 0.0f;
        float maxLostSightTime = 2.5f;
        float repathTimer = 0.0f;

    private:
        void PatrolUpdate();
        void ChaseUpdate();

        bool CanSeePlayer() const;
        bool IsCaught() const;

        void FollowPath(float speed);

        void BuildPathBFS_Random();
        void BuildPathBFS_ToGoal(int gx, int gy);

        int iD(int x, int y) const {
            return y * tileMap->width + x;
        }

        Engine::Vector2 WorldFromTile(int x, int y) const {
            return Engine::Vector2((float)x + 0.5f, (float)y + 0.5f);
        }
    };

}
