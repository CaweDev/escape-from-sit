#include "SecurityGuard.hpp"
#include "Engine.hpp"
#include "Engine/Core/Time.hpp"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>

namespace Game {

    bool SecurityGuard::seeded = false;

    void SecurityGuard::Initialize() {
        if (!seeded) {
            std::srand((unsigned)std::time(nullptr));
            seeded = true;
        }
        BuildPathBFS_PatrolExplore();
    }

    void SecurityGuard::OnCCTVSpotted(const Engine::Vector2& worldPos) {
        memory.hasLastSeen = true;
        memory.lastSeenPos = worldPos;
        memory.timeSinceSeen = 0.0f;
        repathTimer = 0.0f;
        searchTimer = 0.0f;
        SetState(GuardState::Chase);
    }

    void SecurityGuard::Update() {
        if (!tileMap || !playerTf || !playerData) return;
        UpdateStateColor();

        // Update visit memory once per tile entered
        int tx, ty;
        GetCurrentTile(tx, ty);
        int tid = iD(tx, ty);
        if (tid != lastTileId) {
            visitCount[tid] += 1;
            lastTileId = tid;
        }

        switch (state) {
        case GuardState::Patrol:
            PatrolUpdate();
            if (CanSeePlayer()) {
                memory.hasLastSeen = true;
                memory.lastSeenPos = { playerTf->position.x, playerTf->position.y };
                memory.timeSinceSeen = 0.0f;
                repathTimer = 0.0f;
                searchTimer = 0.0f;
                SetState(GuardState::Chase);
            }
            break;

        case GuardState::Chase:
            ChaseUpdate();
            break;

        case GuardState::Search:
            SearchUpdate();
            break;

        case GuardState::Catch:
            std::cout << "\nYOU GOT CAUGHT. GAME OVER\n";
            break;
        }

        // optional debug
        std::cout << "\r[Guard FSM] "
            << StateToString(state)
            << " Guard=(" << gameObject->transform->position.x << "," << gameObject->transform->position.y << ") "
            << "Player=(" << playerTf->position.x << "," << playerTf->position.y << ") "
            << "LastSeen=" << (memory.hasLastSeen ? "yes" : "no")
            << " SeenT=" << memory.timeSinceSeen
            << "      " << std::flush;
    }

    void SecurityGuard::SetState(GuardState newState) {
        if (state == newState) return;
        std::cout << "\n[Guard FSM] " << StateToString(state) << " -> " << StateToString(newState) << "\n";
        state = newState;
    }

    const char* SecurityGuard::StateToString(GuardState s) const {
        switch (s) {
        case GuardState::Patrol: return "Patrol";
        case GuardState::Chase:  return "Chase";
        case GuardState::Search: return "Search";
        case GuardState::Catch:  return "Catch";
        default: return "?";
        }
    }

    void SecurityGuard::PatrolUpdate() {
        // If no path, pick a NEW exploration goal (least visited)
        if (path.empty() || pathIndex >= (int)path.size()) {
            BuildPathBFS_PatrolExplore();
            FollowPath(patrolSpeed);
            return;
        }

        FollowPath(patrolSpeed);

        // If we reached end soon, choose another exploration goal
        if (pathIndex >= (int)path.size()) {
            BuildPathBFS_PatrolExplore();
        }
    }

    void SecurityGuard::ChaseUpdate() {
        if (IsCaught()) {
            SetState(GuardState::Catch);
            if (playerData) { // update player state forgame over ui check
                playerData->Catch();
            }
            return;
        }

        bool seesPlayer = CanSeePlayer();
        if (seesPlayer) {
            memory.lastSeenPos = { playerTf->position.x, playerTf->position.y };
            memory.hasLastSeen = true;
            memory.timeSinceSeen = 0.0f;
        }
        else {
            memory.timeSinceSeen += Engine::Time::deltaTime;
        }

        repathTimer -= Engine::Time::deltaTime;
        if (repathTimer <= 0.0f && memory.hasLastSeen) {
            BuildPathBFS_ToGoal((int)memory.lastSeenPos.x, (int)memory.lastSeenPos.y);
            repathTimer = 0.35f;
        }

        if (!path.empty()) {
            FollowPath(chaseSpeed);
        }

        // Lost player -> Search instead of instantly Patrol
        if (!seesPlayer && memory.timeSinceSeen >= maxLostSightTime) {
            searchTimer = 0.0f;
            SetState(GuardState::Search);
            // build path to last seen once
            if (memory.hasLastSeen) {
                BuildPathBFS_ToGoal((int)memory.lastSeenPos.x, (int)memory.lastSeenPos.y);
            }
            return;
        }
    }

    void SecurityGuard::SearchUpdate() {
        // If player seen again, chase
        if (CanSeePlayer()) {
            memory.lastSeenPos = { playerTf->position.x, playerTf->position.y };
            memory.hasLastSeen = true;
            memory.timeSinceSeen = 0.0f;
            repathTimer = 0.0f;
            SetState(GuardState::Chase);
            return;
        }

        searchTimer += Engine::Time::deltaTime;

        // Move slowly along current search path
        if (!path.empty() && pathIndex < (int)path.size()) {
            FollowPath(searchSpeed);
        }
        else {
            // wander locally: pick a nearby exploration goal (still least visited but closer)
            // temporarily reduce min distance to avoid running away
            float oldMin = patrolMinGoalDist;
            patrolMinGoalDist = 2.0f;
            BuildPathBFS_PatrolExplore();
            patrolMinGoalDist = oldMin;
        }

        if (searchTimer >= searchDuration) {
            memory.hasLastSeen = false;
            memory.timeSinceSeen = 0.0f;
            BuildPathBFS_PatrolExplore();
            SetState(GuardState::Patrol);
        }
    }

    bool SecurityGuard::CanSeePlayer() const {
        float dx = playerTf->position.x - gameObject->transform->position.x;
        float dy = playerTf->position.y - gameObject->transform->position.y;
        return (dx * dx + dy * dy) <= detectRadius * detectRadius;
    }

    bool SecurityGuard::IsCaught() const {
        float dx = playerTf->position.x - gameObject->transform->position.x;
        float dy = playerTf->position.y - gameObject->transform->position.y;
        return (dx * dx + dy * dy) <= catchRadius * catchRadius;
    }

    float SecurityGuard::DistSqTo(float x, float y) const {
        float dx = x - gameObject->transform->position.x;
        float dy = y - gameObject->transform->position.y;
        return dx * dx + dy * dy;
    }

    void SecurityGuard::GetCurrentTile(int& tx, int& ty) const {
        tx = (int)gameObject->transform->position.x;
        ty = (int)gameObject->transform->position.y;
        tx = std::clamp(tx, 0, tileMap->width - 1);
        ty = std::clamp(ty, 0, tileMap->height - 1);
    }

    Engine::Door* SecurityGuard::FindDoorAtTile(int tx, int ty) const {
        auto* scene = Engine::SceneManager::GetActiveScene();
        if (!scene) return nullptr;

        for (auto& obj : scene->GetGameObjects()) {
            if (!obj || !obj->active) continue;
            auto* door = obj->GetComponent<Engine::Door>();
            if (!door) continue;
            if (door->tileX == tx && door->tileY == ty) {
                return door;
            }
        }
        return nullptr;
    }

    void SecurityGuard::UpdateDoorAuto() {
        // Release if door no longer next step
        if (heldDoor) {
            int doorTx = heldDoor->tileX;
            int doorTy = heldDoor->tileY;

            if (pathIndex < (int)path.size()) {
                int nextTx = (int)path[pathIndex].x;
                int nextTy = (int)path[pathIndex].y;

                if (!(nextTx == doorTx && nextTy == doorTy)) {
                    heldDoor->ReleaseOpen(this);
                    heldDoor = nullptr;
                }
            }
            else {
                heldDoor->ReleaseOpen(this);
                heldDoor = nullptr;
            }
        }

        if (pathIndex >= (int)path.size()) return;

        int nextTx = (int)path[pathIndex].x;
        int nextTy = (int)path[pathIndex].y;

        Engine::Door* door = FindDoorAtTile(nextTx, nextTy);
        if (!door) return;

        if (heldDoor != door) {
            heldDoor = door;
            heldDoor->RequestOpen(this);
        }
    }

    void SecurityGuard::FollowPath(float speed) {
        auto rb = gameObject->GetComponent<Engine::Rigidbody2D>();

        if (pathIndex >= (int)path.size()) {
            if (rb) rb->velocity = Engine::Vector2(0.0f, 0.0f);
            return;
        }

        UpdateDoorAuto();

        auto& pos = gameObject->transform->position;
        auto target = path[pathIndex];

        Engine::Vector2 delta(target.x - pos.x, target.y - pos.y);
        float d2 = delta.x * delta.x + delta.y * delta.y;

        if (d2 <= arriveDistance * arriveDistance) {
            pathIndex++;
            if (pathIndex >= (int)path.size() && rb) {
                rb->velocity = Engine::Vector2(0.0f, 0.0f);
            }
            return;
        }

        float d = std::sqrt(d2);
        if (d <= 0.00001f) {
            if (rb) rb->velocity = Engine::Vector2(0.0f, 0.0f);
            return;
        }

        Engine::Vector2 dir(delta.x / d, delta.y / d);

        if (rb) {
            rb->velocity = Engine::Vector2(dir.x * speed, dir.y * speed);
        }
        else {
            pos.x += dir.x * speed * Engine::Time::deltaTime;
            pos.y += dir.y * speed * Engine::Time::deltaTime;
        }
    }

    void SecurityGuard::BuildPathBFS_ToGoal(int gx, int gy) {
        path.clear();
        pathIndex = 0;

        int sx = (int)gameObject->transform->position.x;
        int sy = (int)gameObject->transform->position.y;

        gx = std::clamp(gx, 0, tileMap->width - 1);
        gy = std::clamp(gy, 0, tileMap->height - 1);

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

                if (nx < 0 || ny < 0 || nx >= tileMap->width || ny >= tileMap->height) continue;
                if (!tileMap->IsWalkable(nx + 0.5f, ny + 0.5f)) continue;

                int nid = iD(nx, ny);
                if (parent[nid] != -1) continue;

                parent[nid] = cur;
                q.push(nid);
            }
        }

        if (parent[goal] == -1) return; // no path

        int cur = goal;
        while (cur != start) {
            int x = cur % tileMap->width;
            int y = cur / tileMap->width;
            path.push_back(WorldFromTile(x, y));
            cur = parent[cur];
            if (cur < 0) { path.clear(); return; }
        }

        std::reverse(path.begin(), path.end());
    }

    void SecurityGuard::UpdateStateColor()
    {
        auto* sr = gameObject->GetComponent<Engine::SpriteRenderer>();
        if (!sr) return;

        Engine::Color c;

        switch (state)
        {
        case GuardState::Patrol: c = Engine::Color(0.2f, 0.4f, 1.0f, 1.0f); break; // Blue
        case GuardState::Search: c = Engine::Color(1.0f, 0.9f, 0.2f, 1.0f); break; // Yellow
        case GuardState::Chase:  c = Engine::Color(1.0f, 0.2f, 0.2f, 1.0f); break; // Red
        default:                c = Engine::Color(1.0f, 1.0f, 1.0f, 1.0f); break; // White
        }

        // Try color first
        sr->color = c;

        // If your engine uses tint instead, swap to:
        // sr->tint = c;
    }
    void SecurityGuard::BuildPathBFS_PatrolExplore() {
        if (!tileMap) return;

        int bestX = -1, bestY = -1;
        int bestScore = 1e9;

        // If map is small or visitCount empty, still works.
        for (int i = 0; i < patrolSampleCount; ++i) {
            int gx = std::rand() % tileMap->width;
            int gy = std::rand() % tileMap->height;

            if (!tileMap->IsWalkable(gx + 0.5f, gy + 0.5f)) continue;

            // enforce min distance so it doesn't keep staying in same corridor
            float d2 = DistSqTo((float)gx + 0.5f, (float)gy + 0.5f);
            if (d2 < patrolMinGoalDist * patrolMinGoalDist) continue;

            int id = iD(gx, gy);
            int score = 0;
            auto it = visitCount.find(id);
            if (it != visitCount.end()) score = it->second;

            // pick least visited; tie-break with distance (prefer farther)
            if (score < bestScore) {
                bestScore = score;
                bestX = gx;
                bestY = gy;
            }
        }

        // fallback: if sampling failed, just pick any walkable
        if (bestX < 0) {
            for (int tries = 0; tries < 200; ++tries) {
                int gx = std::rand() % tileMap->width;
                int gy = std::rand() % tileMap->height;
                if (tileMap->IsWalkable(gx + 0.5f, gy + 0.5f)) {
                    bestX = gx; bestY = gy;
                    break;
                }
            }
        }

        if (bestX >= 0) {
            BuildPathBFS_ToGoal(bestX, bestY);
        }
    }

} // namespace Game
