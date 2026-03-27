#include "NPC.hpp"
#include <cmath>
#include <algorithm>
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ConfigManager.hpp"
// HUD notification
#include "Game/Scenes/FPSScene/UI/PlayerHUD.hpp"

namespace
{
    struct NPCConfig : public Engine::IConfig
    {
        float dtFallback = 0.016f;
        float annoyingRepathInterval = 0.25f;
        float headacheRampSeconds = 10.0f;
        float headachePulseDuration = 0.12f;
        float headachePulseBaseIntensity = 0.05f;
        float headacheHudDuration = 3.0f;

        float bullyPushShakeDuration = 0.8f;
        float bullyPushShakeIntensity = 0.75f;
        int searchWanderRadius = 2;

        float sabotageInvestigateDuration = 30.0f;
        float terminalRepairStartDistance = 2.5f;

        void FromJson(const Json::Value& j) override
        {
            dtFallback = j.get("dtFallback", dtFallback).asFloat();
            annoyingRepathInterval = j.get("annoyingRepathInterval", annoyingRepathInterval).asFloat();
            headacheRampSeconds = j.get("headacheRampSeconds", headacheRampSeconds).asFloat();
            headachePulseDuration = j.get("headachePulseDuration", headachePulseDuration).asFloat();
            headachePulseBaseIntensity = j.get("headachePulseBaseIntensity", headachePulseBaseIntensity).asFloat();
            headacheHudDuration = j.get("headacheHudDuration", headacheHudDuration).asFloat();
            bullyPushShakeDuration = j.get("bullyPushShakeDuration", bullyPushShakeDuration).asFloat();
            bullyPushShakeIntensity = j.get("bullyPushShakeIntensity", bullyPushShakeIntensity).asFloat();
            searchWanderRadius = j.get("searchWanderRadius", searchWanderRadius).asInt();
            sabotageInvestigateDuration = j.get("sabotageInvestigateDuration", sabotageInvestigateDuration).asFloat();
            terminalRepairStartDistance = j.get("terminalRepairStartDistance", terminalRepairStartDistance).asFloat();
        }
    };

    NPCConfig g_npcConfig;
    bool g_npcConfigLoaded = false;

    const NPCConfig& GetNPCConfig()
    {
        if (!g_npcConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/npc.json", g_npcConfig);
            if (g_npcConfig.dtFallback <= 0.0f) g_npcConfig.dtFallback = 0.016f;
            if (g_npcConfig.annoyingRepathInterval <= 0.0f) g_npcConfig.annoyingRepathInterval = 0.25f;
            if (g_npcConfig.headacheRampSeconds <= 0.01f) g_npcConfig.headacheRampSeconds = 10.0f;
            if (g_npcConfig.searchWanderRadius < 1) g_npcConfig.searchWanderRadius = 1;
            if (g_npcConfig.terminalRepairStartDistance < 0.1f) g_npcConfig.terminalRepairStartDistance = 0.1f;
            g_npcConfigLoaded = true;
        }
        return g_npcConfig;
    }
}

namespace Game
{
    void NPC::Initialize()
    {
        m_rb = gameObject->GetComponent<Engine::Rigidbody2D>();
        if (playerTf && playerTf->gameObject)
            playerRb = playerTf->gameObject->GetComponent<Engine::Rigidbody2D>();
        activeTimer = 0.0f;
        repathTimer = 0.0f;
        path.clear();
        pathIndex = 0;
        followTimer = 0.0f;
        pulseTimer = 0.0f;
        headacheNotified = false;
    }

    void NPC::Update()
    {
        if (!tileMap) return;

        float dt = Engine::Time::deltaTime;
        if (dt <= 0.0f) dt = GetNPCConfig().dtFallback;

        switch (type)
        {
        case NPCType::Annoying: UpdateAnnoying(dt); break;
        case NPCType::Cleaner:  UpdateCleaner(dt);  break;
        case NPCType::Bully:    UpdateBully(dt);    break;
        }
    }

    void NPC::UpdateAnnoying(float dt)
    {
        if (PlayerInRange() && playerTf && !Player::isInvisible)
        {
            // NPC is actively following / annoying the player
            activeTimer = blockTime;

            // accumulate follow time for headache effect
            followTimer += dt;

            repathTimer -= dt;
            if (path.empty() || pathIndex >= (int)path.size() || repathTimer <= 0.0f)
            {
                int nx, ny; GetCurrentTile(nx, ny);
                int px = (int)playerTf->position.x;
                int py = (int)playerTf->position.y;

                int gx = nx + (px > nx ? 1 : (px < nx ? -1 : 0));
                int gy = ny + (py > ny ? 1 : (py < ny ? -1 : 0));

                if (tileMap->IsWalkable((float)gx + 0.5f, (float)gy + 0.5f))
                    BuildPathBFS_ToGoal(gx, gy);
                else
                    RequestGoalInZone_BiasedNearPlayer();

                repathTimer = GetNPCConfig().annoyingRepathInterval; 
            }

            if (activeTimer > 0.0f)
            {
                activeTimer -= dt;
                FollowPath(dt);
            }

            // Pounding / headache effect: start after threshold and pulse periodically
            if (followTimer >= headacheThreshold && camShake)
            {
                pulseTimer -= dt;
                if (pulseTimer <= 0.0f)
                {
                    pulseTimer = pulseInterval;

                    // intensity scales with time beyond threshold, clamped
                    float over = followTimer - headacheThreshold;
                    float t = std::fmin(1.0f, over / GetNPCConfig().headacheRampSeconds);
                    float intensity = GetNPCConfig().headachePulseBaseIntensity + t * headacheIntensityMax;

                    // short, pronounced pulse (throbbing) to simulate pounding headache
                    camShake->Pulse(GetNPCConfig().headachePulseDuration, intensity);

                    // HUD feedback once when pounding begins (player HUD may be null in tests)
                    PlayerHUD* hud = nullptr;
                    if (playerTf && playerTf->gameObject)
                        hud = playerTf->gameObject->GetComponent<PlayerHUD>();
                    if (hud && !headacheNotified) {
                        hud->ShowHackStatus("You feel a pounding headache...", GetNPCConfig().headacheHudDuration);
                        headacheNotified = true;
                    }
                }
            }
            // Previously played one-shot SFX here. Sound is now managed by the scene
            // via a distance-loop so it will stop when player leaves range.
            return;
        }

        // Out of range: decay follow timer and resume normal patrol behavior
        followTimer -= dt * 2.0f; // faster decay when not following
        if (followTimer < 0.0f) followTimer = 0.0f;
        pulseTimer = 0.0f;
        headacheNotified = false;

        repathTimer -= dt;
        if (path.empty() || pathIndex >= (int)path.size() || repathTimer <= 0.0f)
        {
            RequestGoalInZone_Random();
            repathTimer = patrolRepathTime;
        }
        FollowPath(dt);
    }

    void NPC::UpdateCleaner(float dt)
    {
        repathTimer -= dt;

        if (path.empty() || pathIndex >= (int)path.size() || repathTimer <= 0.0f)
        {
            RequestGoalInZone_Random();
            repathTimer = patrolRepathTime;
        }

        FollowPath(dt);
    }

    void NPC::UpdateBully(float dt)
    {
        if (!playerTf) return;

        // cooldown tick
        bullyCooldown -= dt;

        // start chase when player walks past (enters range) and bully is not cooling down
        if (PlayerInRange() && bullyCooldown <= 0.0f && activeTimer <= 0.0f && !Player::isInvisible)
        {
            activeTimer = bullyChaseTime;   // chasing window
            repathTimer = 0.0f;             // force immediate repath
        }

        // chase phase
        if (activeTimer > 0.0f)
        {
            activeTimer -= dt;

            // repath toward player every interval
            repathTimer -= dt;
            if (repathTimer <= 0.0f)
            {
                int px = (int)playerTf->position.x;
                int py = (int)playerTf->position.y;
                BuildPathBFS_ToGoal(px, py);
                repathTimer = bullyRepathInterval;
            }

  
            float dx = playerTf->position.x - transform->position.x;
            float dy = playerTf->position.y - transform->position.y;
            float d2 = dx * dx + dy * dy;

            if (d2 <= bullyPushRadius * bullyPushRadius)
            {
                float len = std::sqrt(d2);
                if (len < 0.0001f) len = 0.0001f;

              
                float nx = dx / len;
                float ny = dy / len;

              
                Engine::Vector2 impulse(nx * bullyPushStrength, ny * bullyPushStrength);

                if (playerRb)
                {
                    
                    playerRb->velocity.x = nx * bullyPushStrength;
                    playerRb->velocity.y = ny * bullyPushStrength;
                }
                else
                {
                   
                    playerTf->position.x += nx * bullyPushDistance;
                    playerTf->position.y += ny * bullyPushDistance;
                }
                if (camShake)
                    camShake->Shake(GetNPCConfig().bullyPushShakeDuration, GetNPCConfig().bullyPushShakeIntensity);
                activeTimer = 0.0f;
                bullyCooldown = bullyCooldownTime;
                path.clear();
                pathIndex = 0;
                if (m_rb) m_rb->velocity = Engine::Vector2(0, 0);
                // Play push SFX
                Engine::SoundManager::PlaySFX("push");
                return;
            }

            FollowPath(dt);
            return;
        }
    }

    bool NPC::PlayerInRange() const
    {
        if (!playerTf) return false;
        float dx = playerTf->position.x - transform->position.x;
        float dy = playerTf->position.y - transform->position.y;
        return (dx * dx + dy * dy) <= (triggerRadius * triggerRadius);
    }

    void NPC::RequestGoalInZone_Random()
    {
        if (!m_pickGoalInZone) return;

        int gx = 0, gy = 0;
        if (!m_pickGoalInZone(zoneId, gx, gy)) return;

        BuildPathBFS_ToGoal(gx, gy);
    }

    void NPC::RequestGoalInZone_BiasedNearPlayer()
    {
        if (!m_pickGoalInZone || !playerTf) { RequestGoalInZone_Random(); return; }

        int bestX = 0, bestY = 0;
        float bestD2 = 1e30f;
        bool found = false;

        for (int tries = 0; tries < 25; ++tries)
        {
            int gx = 0, gy = 0;
            if (!m_pickGoalInZone(zoneId, gx, gy)) continue;

            float d2 = DistSq((float)gx + 0.5f, (float)gy + 0.5f,
                playerTf->position.x, playerTf->position.y);

            if (d2 < bestD2)
            {
                bestD2 = d2;
                bestX = gx;
                bestY = gy;
                found = true;
            }
        }

        if (!found) return;
        BuildPathBFS_ToGoal(bestX, bestY);
    }

    void NPC::FollowPath(float dt)
    {
        if (path.empty() || pathIndex >= (int)path.size())
        {
            if (m_rb) m_rb->velocity = Engine::Vector2(0, 0);
            return;
        }

        Engine::Vector2 tgt = path[pathIndex];

        float dx = tgt.x - transform->position.x;
        float dy = tgt.y - transform->position.y;
        float d2 = dx * dx + dy * dy;

        if (d2 <= arriveDistance * arriveDistance)
        {
            pathIndex++;
            return;
        }

        float len = std::sqrt(d2);
        if (len > 0.0001f)
        {
            dx /= len;
            dy /= len;
        }

        Engine::Vector2 vel(dx * speed, dy * speed);

        if (m_rb)
        {
            m_rb->velocity = vel;  // move with physics
        }
        else
        {
            transform->position.x += vel.x * dt;
            transform->position.y += vel.y * dt;
        }
    }

    void NPC::BuildPathBFS_ToGoal(int gx, int gy)
    {
        int sx, sy;
        GetCurrentTile(sx, sy);

        if (sx == gx && sy == gy) return;

        const int W = tileMap->width;
        const int H = tileMap->height;

        auto inBounds = [&](int x, int y) {
            return x >= 0 && x < W && y >= 0 && y < H;
            };

        auto walkable = [&](int x, int y) {
            return tileMap->IsWalkable((float)x + 0.5f, (float)y + 0.5f);
            };

        std::queue<std::pair<int, int>> q;
        std::unordered_map<int, int> cameFrom;

        int startId = iD(sx, sy);
        int goalId = iD(gx, gy);

        q.push({ sx, sy });
        cameFrom[startId] = startId;

        const int dirs[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };

        bool found = false;

        while (!q.empty())
        {
            auto [cx, cy] = q.front();
            q.pop();

            int cid = iD(cx, cy);
            if (cid == goalId) { found = true; break; }

            for (auto const& d : dirs)
            {
                int nx = cx + d[0];
                int ny = cy + d[1];

                if (!inBounds(nx, ny)) continue;
                if (!walkable(nx, ny)) continue;

                int nid = iD(nx, ny);
                if (cameFrom.find(nid) != cameFrom.end()) continue;

                cameFrom[nid] = cid;
                q.push({ nx, ny });
            }
        }

        if (!found) return;

        std::vector<Engine::Vector2> newPath;
        int cur = goalId;

        while (cur != startId)
        {
            int x = cur % W;
            int y = cur / W;
            newPath.push_back(WorldFromTile(x, y));
            cur = cameFrom[cur];
        }

        std::reverse(newPath.begin(), newPath.end());
        path = std::move(newPath);
        pathIndex = 0;
    }

    void NPC::GetCurrentTile(int& tx, int& ty) const
    {
        tx = (int)transform->position.x;
        ty = (int)transform->position.y;
    }

    float NPC::DistSq(float ax, float ay, float bx, float by) const
    {
        float dx = ax - bx;
        float dy = ay - by;
        return dx * dx + dy * dy;
    }
}