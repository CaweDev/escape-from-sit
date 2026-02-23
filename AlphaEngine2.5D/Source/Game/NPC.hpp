#pragma once

#include "Engine/Core/Component.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include <vector>
#include <queue>
#include <unordered_map>
#include <functional>

namespace Game
{
    enum class NPCType { Annoying, Cleaner, Bully };

    class NPC : public Engine::Component
    {
    public:
        Engine::TileMap* tileMap = nullptr;
        Engine::Transform* playerTf = nullptr;
        Engine::Rigidbody2D* m_rb = nullptr; 
        NPCType type = NPCType::Annoying;

        // NOTE: your FPSScene zones are 1..4
        int zoneId = 1;

        float speed = 1.0f;
        float arriveDistance = 0.10f;

        // Annoying trigger
        float triggerRadius = 2.2f;
        float blockTime = 1.2f;

        // Cleaner patrol
        float patrolRepathTime = 2.0f;

        void Initialize() override;
        void Update() override;

        // FPSScene passes: (zoneId, outX, outY) -> bool
        void SetPickGoalInZone(std::function<bool(int, int&, int&)> fn) { m_pickGoalInZone = fn; }

    private:
        // Path
        std::vector<Engine::Vector2> path;
        int pathIndex = 0;

        // Timers
        float activeTimer = 0.0f; // Annoying
        float repathTimer = 0.0f; // Cleaner

        std::function<bool(int, int&, int&)> m_pickGoalInZone;

    private:
        // Update per type
        void UpdateAnnoying(float dt);
        void UpdateCleaner(float dt);
        void UpdateBully(float dt);

        bool PlayerInRange() const;

        // Goals
        void RequestGoalInZone_BiasedNearPlayer();
        void RequestGoalInZone_Random();

        // Movement
        void FollowPath(float dt);

        // BFS
        void BuildPathBFS_ToGoal(int gx, int gy);

        // Helpers
        void GetCurrentTile(int& tx, int& ty) const;
        int iD(int x, int y) const { return y * tileMap->width + x; }
        Engine::Vector2 WorldFromTile(int x, int y) const { return Engine::Vector2((float)x + 0.5f, (float)y + 0.5f); }
        float DistSq(float ax, float ay, float bx, float by) const;
    };
}