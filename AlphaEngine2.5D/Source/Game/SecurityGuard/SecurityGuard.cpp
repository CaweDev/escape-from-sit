// =======================
// SecurityGuard.cpp
// =======================
#include "SecurityGuard.hpp"
#include "Game/CCTV/CCTVTerminal.hpp"
#include "Engine.hpp"
#include "Engine/Core/Time.hpp"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <queue>

namespace Game {

    bool SecurityGuard::seeded = false;

    void SecurityGuard::Initialize() {
        if (!seeded) {
            std::srand((unsigned)std::time(nullptr));
            seeded = true;
        }

        transform = gameObject->transform;
        BuildPathBFS_PatrolExplore();
    }

    void SecurityGuard::OnCCTVSpotted(const Engine::Vector2& worldPos) {
        if (state != GuardState::Patrol) return;

        memory.hasLastSeen = true;
        memory.lastSeenPos = worldPos;
        memory.timeSinceSeen = 0.0f;

        cctvPursuitTimer = cctvPursuitDuration;

        repathTimer = 0.0f;
        searchTimer = 0.0f;

        BuildPathBFS_ToGoal((int)worldPos.x, (int)worldPos.y);
        SetState(GuardState::Chase);
    }

    void SecurityGuard::Update() {
        if (!tileMap || !playerTf || !playerData) return;
        UpdateStateColor();

        int tx, ty;
        GetCurrentTile(tx, ty);
        int tid = iD(tx, ty);
        if (tid != lastTileId) {
            visitCount[tid] += 1;
            lastTileId = tid;
        }

        if (zoneCommitTimer > 0.0f)
            zoneCommitTimer -= Engine::Time::deltaTime;

        switch (state) {
        case GuardState::Patrol:
            PatrolUpdate();
            if (CanSeePlayer()) {
                memory.hasLastSeen = true;
                memory.lastSeenPos = { playerTf->position.x, playerTf->position.y };
                memory.timeSinceSeen = 0.0f;
                repathTimer = 0.0f;
                searchTimer = 0.0f;
                cctvPursuitTimer = 0.0f;
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
    }

    void SecurityGuard::SetState(GuardState newState) {
        if (state == newState) return;
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

    void SecurityGuard::SetPatrolZoneOverride(int zoneId)
    {
        if (m_hasForcedZone && m_forcedZoneId == zoneId)
            return;

        m_hasForcedZone = (zoneId > 0);
        m_forcedZoneId = zoneId;

        zoneCommitTimer = zoneCommitDuration;
    }

    void SecurityGuard::SetAssistZone(int zoneId)
    {
        assisting = true;
        fullEscalation = false;
        SetPatrolZoneOverride(zoneId);
    }

    void SecurityGuard::ClearAssist()
    {
        assisting = false;
        if (!fullEscalation)
            SetPatrolZoneOverride(homeZoneId);
    }

    void SecurityGuard::SetFullEscalation(int zoneId)
    {
        fullEscalation = true;
        assisting = false;
        SetPatrolZoneOverride(zoneId);
    }

    void SecurityGuard::ClearFullEscalation()
    {
        fullEscalation = false;
        if (!assisting)
            SetPatrolZoneOverride(homeZoneId);
    }

    void SecurityGuard::PatrolUpdate()
    {
        if (!zoneMgr)
        {
            if (path.empty() || pathIndex >= (int)path.size())
                BuildPathBFS_PatrolExplore();

            FollowPath(patrolSpeed);
            return;
        }

        int desiredZoneId = -1;
        int overrideZone = GetPatrolZoneOverride();
        if (overrideZone > 0)
            desiredZoneId = overrideZone;
        else
            desiredZoneId = homeZoneId;

        learnRepathTimer -= Engine::Time::deltaTime;

        bool shouldRepath = (path.empty() || pathIndex >= (int)path.size());
        bool canChangeZone = (zoneCommitTimer <= 0.0f);

        if (shouldRepath || (learnRepathTimer <= 0.0f && canChangeZone))
        {
            int gx = 0, gy = 0;
            if (zoneMgr->PickRandomWalkableInZone(desiredZoneId, gx, gy))
            {
                BuildPathBFS_ToGoal(gx, gy);
            }
            else
            {
                BuildPathBFS_PatrolExplore();
            }

            learnRepathTimer = chooseInterval;
        }

        FollowPath(patrolSpeed);
    }

    void SecurityGuard::ChaseUpdate() {
        if (IsCaught()) {
            if (playerData) {
                playerData->Catch();
                if (playerData->IsCaught()) SetState(GuardState::Catch);
            }
            return;
        }

        const bool seesPlayer = CanSeePlayer();
        if (seesPlayer) {
            memory.lastSeenPos = { playerTf->position.x, playerTf->position.y };
            memory.hasLastSeen = true;
            memory.timeSinceSeen = 0.0f;
            cctvPursuitTimer = 0.0f;
        }
        else {
            memory.timeSinceSeen += Engine::Time::deltaTime;
            if (cctvPursuitTimer > 0.0f)
                cctvPursuitTimer -= Engine::Time::deltaTime;
        }

        repathTimer -= Engine::Time::deltaTime;
        if (repathTimer <= 0.0f && memory.hasLastSeen) {
            BuildPathBFS_ToGoal((int)memory.lastSeenPos.x, (int)memory.lastSeenPos.y);
            repathTimer = 0.35f;
        }

        if (!path.empty()) {
            FollowPath(chaseSpeed);
        }

        if (!seesPlayer && memory.timeSinceSeen >= maxLostSightTime && cctvPursuitTimer <= 0.0f) {
            searchTimer = 0.0f;
            SetState(GuardState::Search);
            if (memory.hasLastSeen) {
                BuildPathBFS_ToGoal((int)memory.lastSeenPos.x, (int)memory.lastSeenPos.y);
            }
            return;
        }
    }

    void SecurityGuard::SearchUpdate() {
    // If investigating sabotage, prioritize terminal repair over chasing player
    if (investigatingSabotage) {
        // Only interrupt for immediate catch
        if (IsCaught()) {
            investigatingSabotage = false;
            SetState(GuardState::Catch);
            if (playerData) playerData->Catch();
            return;
        }
        // Otherwise stay focused on reaching terminal even if player spotted
    }
    else if (CanSeePlayer()) {
        // Normal search behavior: spot player ? chase
        memory.lastSeenPos = { playerTf->position.x, playerTf->position.y };
        memory.hasLastSeen = true;
        memory.timeSinceSeen = 0.0f;
        repathTimer = 0.0f;
        cctvPursuitTimer = 0.0f;
        SetState(GuardState::Chase);
        return;
    }

        searchTimer += Engine::Time::deltaTime;

        // Handle sabotage investigation
        if (investigatingSabotage && cctvTerminals)
        {
            sabotageInvestigateTimer -= Engine::Time::deltaTime;
            
            // Check if we're at destination
            bool atDestination = (path.empty() || pathIndex >= (int)path.size());
            
            if (atDestination)
            {
                // Find nearby disabled terminal and start repair
                bool foundTerminal = false;
                for (auto* terminal : *cctvTerminals)
                {
                    if (!terminal || !terminal->isDisabled) continue;
                    
                    Engine::Vector2 termPos(terminal->transform->position.x, terminal->transform->position.y);
                    Engine::Vector2 guardPos(transform->position.x, transform->position.y);
                    
                    float dx = termPos.x - guardPos.x;
                    float dy = termPos.y - guardPos.y;
                    float dist = std::sqrt(dx*dx + dy*dy);
                    
                    if (dist <= 2.5f && !terminal->guardIsRepairing)
                    {
                        terminal->StartRepair(this);
                        foundTerminal = true;
                        std::cout << "[Guard] Starting repair on terminal at (" 
                                  << termPos.x << ", " << termPos.y << ")\n";
                        break;
                    }
                }
                
                // If no terminal found at destination, search nearby area
                if (!foundTerminal)
                {
                    // Path to a random nearby location to continue searching
                    int gx = (int)transform->position.x + (std::rand() % 5 - 2);
                    int gy = (int)transform->position.y + (std::rand() % 5 - 2);
                    if (tileMap && tileMap->IsWalkable((float)gx + 0.5f, (float)gy + 0.5f))
                    {
                        BuildPathBFS_ToGoal(gx, gy);
                    }
                }
            }
            
            // Timeout: return to patrol
            if (sabotageInvestigateTimer <= 0.0f)
            {
                investigatingSabotage = false;
                sabotageInvestigateTimer = 0.0f;
                BuildPathBFS_PatrolExplore();
                SetState(GuardState::Patrol);
                std::cout << "[Guard] Investigation timeout, returning to patrol\n";
                return;
            }
        }

        if (!path.empty() && pathIndex < (int)path.size()) {
            FollowPath(searchSpeed);
        }
        else {
            float oldMin = patrolMinGoalDist;
            patrolMinGoalDist = 2.0f;
            BuildPathBFS_PatrolExplore();
            patrolMinGoalDist = oldMin;
        }

        if (searchTimer >= searchDuration) {
            memory.hasLastSeen = false;
            memory.timeSinceSeen = 0.0f;
            investigatingSabotage = false;
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

        if (parent[goal] == -1) return;

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
        case GuardState::Patrol:
            if (fullEscalation)
                c = Engine::Color(1.0f, 0.5f, 0.0f, 1.0f); // orange
            else if (assisting)
                c = Engine::Color(0.5f, 0.8f, 1.0f, 1.0f); // light blue
            else
                c = Engine::Color(0.2f, 0.4f, 1.0f, 1.0f); // blue
            break;
        case GuardState::Search: c = Engine::Color(1.0f, 0.9f, 0.2f, 1.0f); break;
        case GuardState::Chase:  c = Engine::Color(1.0f, 0.2f, 0.2f, 1.0f); break;
        default:                c = Engine::Color(1.0f, 1.0f, 1.0f, 1.0f); break;
        }

        sr->color = c;
    }

    void SecurityGuard::BuildPathBFS_PatrolExplore() {
        if (!tileMap) return;

        int bestX = -1, bestY = -1;
        int bestScore = 1e9;

        for (int i = 0; i < patrolSampleCount; ++i) {
            int gx = std::rand() % tileMap->width;
            int gy = std::rand() % tileMap->height;

            if (!tileMap->IsWalkable(gx + 0.5f, gy + 0.5f)) continue;

            float d2 = DistSqTo((float)gx + 0.5f, (float)gy + 0.5f);
            if (d2 < patrolMinGoalDist * patrolMinGoalDist) continue;

            int id = iD(gx, gy);
            int score = 0;
            auto it = visitCount.find(id);
            if (it != visitCount.end()) score = it->second;

            if (score < bestScore) {
                bestScore = score;
                bestX = gx;
                bestY = gy;
            }
        }

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

    void SecurityGuard::OnCCTVSystemSabotaged(const Engine::Vector2& controlRoomPos, int sabotagedZoneId)
    {
        std::cout << "[Guard Zone " << homeZoneId << "] OnCCTVSystemSabotaged called!\n";
        std::cout << "  Current state: " << StateToString(state) << "\n";
        std::cout << "  Sabotage location: (" << controlRoomPos.x << ", " << controlRoomPos.y << ")\n";
        std::cout << "  Sabotaged zone: " << sabotagedZoneId << "\n";
        
        // Only respond if currently in Patrol state
        if (state != GuardState::Patrol) {
            std::cout << "[Guard] Ignoring sabotage - not in Patrol state\n";
            return;
        }

        // Trigger investigation behavior:
        // 1. Set memory to control room location
        memory.hasLastSeen = true;
        memory.lastSeenPos = controlRoomPos;
        memory.timeSinceSeen = 0.0f;
        investigatingSabotage = true;
        sabotageLocation = controlRoomPos;
        sabotageInvestigateTimer = 30.0f; // Investigate for 30 seconds (matches disabled duration)

        // 2. Path to control room (add small offset to avoid wall placement)
        int targetX = (int)(controlRoomPos.x + 0.5f);
        int targetY = (int)(controlRoomPos.y + 0.5f);
        
        // Ensure target is walkable
        if (tileMap && !tileMap->IsWalkable((float)targetX + 0.5f, (float)targetY + 0.5f)) {
            // Find nearest walkable tile
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    int testX = targetX + dx;
                    int testY = targetY + dy;
                    if (tileMap->IsWalkable((float)testX + 0.5f, (float)testY + 0.5f)) {
                        targetX = testX;
                        targetY = testY;
                        goto foundWalkable;
                    }
                }
            }
            foundWalkable:;
        }
        
        std::cout << "[Guard] Building path to terminal at (" << targetX << ", " << targetY << ")\n";
        BuildPathBFS_ToGoal(targetX, targetY);
        std::cout << "[Guard] Path has " << path.size() << " waypoints\n";

        // 3. Switch to Search state to investigate
        SetState(GuardState::Search);
        searchTimer = 0.0f;

        std::cout << "[Guard Zone " << homeZoneId 
                  << "] INVESTIGATING sabotaged terminal in zone " << sabotagedZoneId 
                  << " at (" << controlRoomPos.x << ", " << controlRoomPos.y << ")\n";
    }

} // namespace Game