#include "SecurityGuard.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>

namespace Game {

    bool SecurityGuard::seeded = false;

    void SecurityGuard::Initialize()
    {
        if (!seeded) {
            std::srand((unsigned)std::time(nullptr));
            seeded = true;
        }

        BuildPathBFS_Random();
    }

    void SecurityGuard::Update()
    {
        if (!tileMap || !playerTf || !playerData) return;
        
        if (hasCctvLastSeen && !CanSeePlayer()) {
            hasLastSeen = true;
            lastSeenPos = cctvLastSeenPos;

            lostSightTimer = 0.0f;
            repathTimer = 0.0f;

            if (state == GuardState::Patrol) {
                state = GuardState::Chase;
            }
            hasCctvLastSeen = false;
        }

        switch (state)
        {
        case GuardState::Patrol:
            PatrolUpdate();

            if (CanSeePlayer()) {
                state = GuardState::Chase;
                hasLastSeen = true;
                lastSeenPos = {
                    playerTf->position.x,
                    playerTf->position.y
                };
                lostSightTimer = 0.0f;
                repathTimer = 0.0f;
            }
            break;

        case GuardState::Chase:
            ChaseUpdate();
            break;

        case GuardState::Catch:
            std::cout << "YOU GOT CAUGHT. GAME OVER\n";
            break;
        }
        //debug 
        std::cout << "\r[Guard FSM] "
            << "State=" << (int)state
            << " Guard=("
            << gameObject->transform->position.x << ", "
            << gameObject->transform->position.y << ") "
            << "Player=("
            << playerTf->position.x << ", "
            << playerTf->position.y << ") "
            << "LastSeen="
            << (hasLastSeen
                ? "(" + std::to_string(lastSeenPos.x) + ", " + std::to_string(lastSeenPos.y) + ")"
                : "(none)")
            << " LostSight=" << lostSightTimer
            << "        "
            << std::flush;
    }

  
    void SecurityGuard::PatrolUpdate()
    {
        if (path.empty() || pathIndex >= (int)path.size()) {
            BuildPathBFS_Random();
            return;
        }

        FollowPath(patrolSpeed);
    }

  
    void SecurityGuard::ChaseUpdate()
    {
        if (IsCaught()) {
            state = GuardState::Catch;
            return;
        }

        bool seesPlayer = CanSeePlayer();

        if (seesPlayer) {
           
            lastSeenPos = {
                playerTf->position.x,
                playerTf->position.y
            };
            hasLastSeen = true;
            lostSightTimer = 0.0f;
        }
        else {
            lostSightTimer += Engine::Time::deltaTime;
        }

        
        repathTimer -= Engine::Time::deltaTime;
        if (repathTimer <= 0.0f && hasLastSeen) {
            BuildPathBFS_ToGoal(
                (int)lastSeenPos.x,
                (int)lastSeenPos.y
            );
            repathTimer = 0.35f;
        }

       
        if (!path.empty()) {
            FollowPath(chaseSpeed);
        }

        if (!seesPlayer && lostSightTimer >= maxLostSightTime) {
            hasLastSeen = false;
            state = GuardState::Patrol;
            BuildPathBFS_Random();
            return;
        }

     
        if (!seesPlayer && (path.empty() || pathIndex >= (int)path.size())) {
            hasLastSeen = false;
            state = GuardState::Patrol;
            BuildPathBFS_Random();
            return;
        }
    }

   
    bool SecurityGuard::CanSeePlayer() const
    {
        float dx = playerTf->position.x - gameObject->transform->position.x;
        float dy = playerTf->position.y - gameObject->transform->position.y;
        return (dx * dx + dy * dy) <= detectRadius * detectRadius;
    }

    bool SecurityGuard::IsCaught() const
    {
        float dx = playerTf->position.x - gameObject->transform->position.x;
        float dy = playerTf->position.y - gameObject->transform->position.y;
        return (dx * dx + dy * dy) <= catchRadius * catchRadius;
    }

    void SecurityGuard::FollowPath(float speed)
    {
        if (pathIndex >= (int)path.size()) return;

        auto& pos = gameObject->transform->position;
        auto target = path[pathIndex];

        Engine::Vector2 delta(target.x - pos.x, target.y - pos.y);
        float d2 = delta.x * delta.x + delta.y * delta.y;

        if (d2 <= arriveDistance * arriveDistance) {
            pathIndex++;
            return;
        }

        float d = std::sqrt(d2);
        Engine::Vector2 dir(delta.x / d, delta.y / d);

        pos.x += dir.x * speed * Engine::Time::deltaTime;
        pos.y += dir.y * speed * Engine::Time::deltaTime;
    }

    void SecurityGuard::BuildPathBFS_Random()
    {
        int gx, gy;
        do {
            gx = std::rand() % tileMap->width;
            gy = std::rand() % tileMap->height;
        } while (!tileMap->IsWalkable(gx + 0.5f, gy + 0.5f));

        BuildPathBFS_ToGoal(gx, gy);
    }

    void SecurityGuard::BuildPathBFS_ToGoal(int gx, int gy)
    {
        path.clear();
        pathIndex = 0;

        int sx = (int)gameObject->transform->position.x;
        int sy = (int)gameObject->transform->position.y;

        std::vector<int> parent(tileMap->width * tileMap->height, -1);
        std::queue<int> q;

        int start = iD(sx, sy);
        int goal = iD(gx, gy);

        parent[start] = start;
        q.push(start);

        const int dX[4] = { 1,-1,0,0 };
        const int dY[4] = { 0,0,1,-1 };

        while (!q.empty()) {
            int cur = q.front(); q.pop();
            if (cur == goal) break;

            int cx = cur % tileMap->width;
            int cy = cur / tileMap->width;

            for (int i = 0; i < 4; ++i) {
                int nx = cx + dX[i];
                int ny = cy + dY[i];

                if (nx < 0 || ny < 0 ||
                    nx >= tileMap->width || ny >= tileMap->height) continue;
                if (!tileMap->IsWalkable(nx + 0.5f, ny + 0.5f)) continue;

                int nid = iD(nx, ny);
                if (parent[nid] != -1) continue;

                parent[nid] = cur;
                q.push(nid);
            }
        }

        int cur = goal;
        while (cur != start) {
            int x = cur % tileMap->width;
            int y = cur / tileMap->width;
            path.push_back(WorldFromTile(x, y));
            cur = parent[cur];
            if (cur < 0) return;
        }

        std::reverse(path.begin(), path.end());
    }

}
