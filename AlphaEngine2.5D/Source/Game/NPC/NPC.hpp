/*!
 * \file AlphaEngine2.5D\Source\Game\NPC\NPC.hpp
 * \brief NPC behavior
 */

#pragma once

#include "Engine/Core/Component.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Game/NPC/CameraShake.hpp"
#include <vector>
#include <queue>
#include <unordered_map>
#include <functional>
#include "Game/Player/Player.hpp"
namespace Game
{
    enum class NPCType
    {
        Cleaner,
        Bully,
        Annoying
    };

    class NPC : public Engine::Component
    {
    public:
        Engine::TileMap* tileMap = nullptr;
        Engine::Transform* playerTf = nullptr;
        Engine::Rigidbody2D* m_rb = nullptr; 
        Engine::Rigidbody2D* playerRb = nullptr; 
        Game::Player* playerData = nullptr;
        Game::CameraShake* camShake = nullptr;
        NPCType type = NPCType::Annoying;

      
        int zoneId = 1;

        float speed = 1.0f;
        float arriveDistance = 0.10f;

        // Annoying trigger
        float triggerRadius = 2.2f;
        float blockTime = 1.2f;

        // Cleaner patrol
        float patrolRepathTime = 2.0f;
        
        bool isDistracted  = false;
        Engine::Vector2 distractionPos;

        void Initialize() override;
        void Update() override;

        // FPSScene passes: (zoneId, outX, outY) -> bool
        void SetPickGoalInZone(std::function<bool(int, int&, int&)> fn) { m_pickGoalInZone = fn; }
        void OnDistracted(const Engine::Vector2 &noisePos);
        void ClearDistraction();

    private:
        // Path
        std::vector<Engine::Vector2> path;
        int pathIndex = 0;

        // Timers
        float activeTimer = 0.0f; // Annoying
        float repathTimer = 0.0f; // Cleaner
        float bullyChaseTime = 1.5f;
        float bullyRepathInterval = 0.25f;
        float bullyCooldownTime = 2.0f;

        float bullyPushRadius = 1.0f;
        float bullyPushStrength = 18.0f;   
        float bullyPushDistance = 1.2f;
        float bullyCooldown = 0.0f;
        std::function<bool(int, int&, int&)> m_pickGoalInZone;

        // Headache / follow tracking (Annoying)
        float followTimer = 0.0f;            // time spent continuously following the player
        float headacheThreshold = 12.0f;     // seconds before pounding starts
        float pulseTimer = 0.0f;             // timer for pounding pulses
        float pulseInterval = 0.6f;          // seconds between pulses
        float headacheIntensityMax = 0.18f;  // max shake intensity from pounding
        bool  headacheNotified = false;      // whether HUD was notified for this follow session

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