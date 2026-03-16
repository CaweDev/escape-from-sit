#include "NPC.hpp"
#include <cmath>
#include <algorithm>
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
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
    }

    void NPC::Update()
    {
        if (!tileMap) return;

        float dt = Engine::Time::deltaTime;
        if (dt <= 0.0f) dt = 0.016f;

        switch (type)
        {
        case NPCType::Annoying: UpdateAnnoying(dt); break;
        case NPCType::Cleaner:  UpdateCleaner(dt);  break;
        case NPCType::Bully:    UpdateBully(dt);    break;
        }
    }

    void NPC::UpdateAnnoying(float dt)
    {
        if (PlayerInRange() && playerTf)
        {
            activeTimer = blockTime;

          
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

                repathTimer = 0.25f; 
            }
        }

        if (activeTimer > 0.0f)
        {
            activeTimer -= dt;
            FollowPath(dt);
        }
        else
        {
            path.clear();
            pathIndex = 0;
        }
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
        if (PlayerInRange() && bullyCooldown <= 0.0f && activeTimer <= 0.0f)
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
                    camShake->Shake(0.8f, 0.75f);
                activeTimer = 0.0f;
                bullyCooldown = bullyCooldownTime;
                path.clear();
                pathIndex = 0;
                if (m_rb) m_rb->velocity = Engine::Vector2(0, 0);
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