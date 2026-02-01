#include "SecurityGuard.hpp"
#include "Engine.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/Scene/Scene.hpp"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <string>

namespace Game {

    bool SecurityGuard::seeded = false;

    
    const char* SecurityGuard::StateToString(GuardState s) const {
        switch (s) {
        case GuardState::Patrol: return "Patrol";
        case GuardState::Chase:  return "Chase";
        case GuardState::Search: return "Search";
        case GuardState::Catch:  return "Catch";
        default: return "Unknown";
        }
    }

    void SecurityGuard::SetState(GuardState newState) {
        if (state != newState) {
            std::cout << "[Guard FSM] " << StateToString(state)
                << " -> " << StateToString(newState) << "\n";
            state = newState;
        }
    }

   
    Engine::Color SecurityGuard::GetColorForState() const {
        switch (state) {
        case GuardState::Patrol:
            return Engine::Color(0.2f, 0.4f, 1.0f, 1.0f); // Blue
        case GuardState::Search:
            return Engine::Color(1.0f, 0.9f, 0.2f, 1.0f); // Yellow
        case GuardState::Chase:
            return Engine::Color(1.0f, 0.2f, 0.2f, 1.0f); // Red
        default:
            return Engine::Color(1.0f, 1.0f, 1.0f, 1.0f); // White
        }
    }

    void SecurityGuard::ApplyStateColor() {
        auto* sr = gameObject->GetComponent<Engine::SpriteRenderer>();
        if (!sr) return;

      
        sr->color = GetColorForState();

    }

  
    void SecurityGuard::Initialize() {
        if (!seeded) {
            std::srand((unsigned)std::time(nullptr));
            seeded = true;
        }
        BuildPathBFS_Random();
    }

    void SecurityGuard::OnCCTVSpotted(const Engine::Vector2& worldPos) {

        memory.hasLastSeen = true;
        memory.lastSeenPos = worldPos;
        memory.timeSinceSeen = 0.0f;

        repathTimer = 0.0f;
        searchTimer = 0.0f;
        searchRepathTimer = 0.0f;

        SetState(GuardState::Search);
        BuildPathBFS_ToGoal((int)memory.lastSeenPos.x, (int)memory.lastSeenPos.y);
    }

    void SecurityGuard::Update() {
        if (!tileMap || !playerTf || !playerData) return;

        switch (state) {
        case GuardState::Patrol:
            PatrolUpdate();
            if (CanSeePlayer()) {
                memory.hasLastSeen = true;
                memory.lastSeenPos = { playerTf->position.x, playerTf->position.y };
                memory.timeSinceSeen = 0.0f;
                repathTimer = 0.0f;
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
            std::cout << "YOU GOT CAUGHT. GAME OVER\n";
            break;
        }

        ApplyStateColor();

        std::cout << "\r[Guard] "
            << "State=" << StateToString(state)
            << " Guard=(" << gameObject->transform->position.x << ", " << gameObject->transform->position.y << ") "
            << " LastSeen=" << (memory.hasLastSeen
                ? "(" + std::to_string(memory.lastSeenPos.x) + "," + std::to_string(memory.lastSeenPos.y) + ")"
                : "(none)")
            << " SinceSeen=" << memory.timeSinceSeen
            << " SearchT=" << searchTimer
            << "      "
            << std::flush;
    }

  
    void SecurityGuard::PatrolUpdate() {
        if (path.empty() || pathIndex >= (int)path.size()) {
            BuildPathBFS_Random();
            return;
        }
        FollowPath(patrolSpeed);
    }

    void SecurityGuard::ChaseUpdate() {
        if (IsCaught()) {
            SetState(GuardState::Catch);
            return;
        }

        const bool seesPlayer = CanSeePlayer();

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

     
        if (!seesPlayer && memory.timeSinceSeen >= maxLostSightTime) {
            memory.timeSinceSeen = 0.0f;
            searchTimer = 0.0f;
            searchRepathTimer = 0.0f;

            SetState(GuardState::Search);

            if (memory.hasLastSeen) BuildPathBFS_ToGoal((int)memory.lastSeenPos.x, (int)memory.lastSeenPos.y);
            else BuildPathBFS_Random();
            return;
        }

      
        if (!seesPlayer && (path.empty() || pathIndex >= (int)path.size())) {
            searchTimer = 0.0f;
            searchRepathTimer = 0.0f;

            SetState(GuardState::Search);

            if (memory.hasLastSeen) BuildPathBFS_ToGoal((int)memory.lastSeenPos.x, (int)memory.lastSeenPos.y);
            else BuildPathBFS_Random();
            return;
        }
    }

    void SecurityGuard::SearchUpdate() {
       
        if (CanSeePlayer()) {
            memory.hasLastSeen = true;
            memory.lastSeenPos = { playerTf->position.x, playerTf->position.y };
            memory.timeSinceSeen = 0.0f;
            repathTimer = 0.0f;
            SetState(GuardState::Chase);
            return;
        }

        searchTimer += Engine::Time::deltaTime;

      
        if (!path.empty() && pathIndex < (int)path.size()) {
            FollowPath(searchSpeed);
        }

        
        const bool needWander = (path.empty() || pathIndex >= (int)path.size());
        searchRepathTimer -= Engine::Time::deltaTime;

        if (needWander && searchRepathTimer <= 0.0f) {
            Engine::Vector2 anchor = memory.hasLastSeen
                ? memory.lastSeenPos
                : Engine::Vector2(gameObject->transform->position.x, gameObject->transform->position.y);

            int ax = (int)anchor.x;
            int ay = (int)anchor.y;

            int gx = std::clamp(ax + ((std::rand() % 7) - 3), 0, tileMap->width - 1);
            int gy = std::clamp(ay + ((std::rand() % 7) - 3), 0, tileMap->height - 1);

            for (int tries = 0; tries < 6; ++tries) {
                if (tileMap->IsWalkable(gx + 0.5f, gy + 0.5f)) break;
                gx = std::clamp(ax + ((std::rand() % 7) - 3), 0, tileMap->width - 1);
                gy = std::clamp(ay + ((std::rand() % 7) - 3), 0, tileMap->height - 1);
            }

            BuildPathBFS_ToGoal(gx, gy);
            searchRepathTimer = 0.6f;
        }

       
        if (searchTimer >= searchDuration) {
            searchTimer = 0.0f;
            memory.hasLastSeen = false;

            SetState(GuardState::Patrol);
            BuildPathBFS_Random();
            return;
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

    Engine::Door* SecurityGuard::FindDoorAtTile(int tx, int ty) const {
        auto* scene = Engine::SceneManager::GetActiveScene();
        if (!scene) return nullptr;

        for (auto& obj : scene->GetGameObjects()) {
            if (!obj || !obj->active) continue;
            auto* door = obj->GetComponent<Engine::Door>();
            if (!door) continue;
            if (door->tileX == tx && door->tileY == ty) return door;
        }
        return nullptr;
    }

    void SecurityGuard::UpdateDoorAuto() {
       
        if (heldDoor) {
            Engine::Vector2 doorPos = heldDoor->GetWorldPosition();
            float dx = doorPos.x - gameObject->transform->position.x;
            float dy = doorPos.y - gameObject->transform->position.y;
            float d2 = dx * dx + dy * dy;

            if (d2 > doorHoldReleaseDist * doorHoldReleaseDist) {
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

 
    void SecurityGuard::BuildPathBFS_Random() {
        int gx, gy;
        do {
            gx = std::rand() % tileMap->width;
            gy = std::rand() % tileMap->height;
        } while (!tileMap->IsWalkable(gx + 0.5f, gy + 0.5f));

        BuildPathBFS_ToGoal(gx, gy);
    }

    void SecurityGuard::BuildPathBFS_ToGoal(int gx, int gy) {
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

                if (nx < 0 || ny < 0 || nx >= tileMap->width || ny >= tileMap->height) continue;
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
            if (cur < 0) return; // no path
        }

        std::reverse(path.begin(), path.end());
    }

} // namespace Game