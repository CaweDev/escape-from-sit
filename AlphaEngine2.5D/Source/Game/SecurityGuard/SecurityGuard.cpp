/*!
 * \file AlphaEngine2.5D\Source\Game\SecurityGuard\SecurityGuard.cpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief [AUTO] Added Doxygen file header
 */

// =======================
// SecurityGuard.cpp
// =======================
#include "SecurityGuard.hpp"
#include "Game/CCTV/CCTVTerminal.hpp"
#include "Engine.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <queue>
#include "Game/Player/PlayerPowerUpController.hpp"

namespace
{
    struct SecurityGuardConfig : public Engine::IConfig
    {
        float chaseRepathInterval = 0.35f;
        float sabotageInvestigateDuration = 30.0f;
        float terminalRepairDistance = 2.5f;
        int fallbackSearchRadius = 2;
        int walkableSearchRadius = 2;
        float doorAutoCloseDelay = 3.0f;
        float doorInteractDistance = 2.5f;
        float defaultDoorFov = 75.0f;
        float chaseLoopVolume = 1.0f;

        void FromJson(const Json::Value& j) override
        {
            chaseRepathInterval = j.get("chaseRepathInterval", chaseRepathInterval).asFloat();
            sabotageInvestigateDuration = j.get("sabotageInvestigateDuration", sabotageInvestigateDuration).asFloat();
            terminalRepairDistance = j.get("terminalRepairDistance", terminalRepairDistance).asFloat();
            fallbackSearchRadius = j.get("fallbackSearchRadius", fallbackSearchRadius).asInt();
            walkableSearchRadius = j.get("walkableSearchRadius", walkableSearchRadius).asInt();
            doorAutoCloseDelay = j.get("doorAutoCloseDelay", doorAutoCloseDelay).asFloat();
            doorInteractDistance = j.get("doorInteractDistance", doorInteractDistance).asFloat();
            defaultDoorFov = j.get("defaultDoorFov", defaultDoorFov).asFloat();
            chaseLoopVolume = j.get("chaseLoopVolume", chaseLoopVolume).asFloat();
        }
    };

    SecurityGuardConfig g_securityGuardConfig;
    bool g_securityGuardConfigLoaded = false;

    const SecurityGuardConfig& GetSecurityGuardConfig()
    {
        if (!g_securityGuardConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/security_guard.json", g_securityGuardConfig);
            if (g_securityGuardConfig.chaseRepathInterval <= 0.0f) g_securityGuardConfig.chaseRepathInterval = 0.35f;
            if (g_securityGuardConfig.terminalRepairDistance < 0.1f) g_securityGuardConfig.terminalRepairDistance = 0.1f;
            if (g_securityGuardConfig.fallbackSearchRadius < 1) g_securityGuardConfig.fallbackSearchRadius = 1;
            if (g_securityGuardConfig.walkableSearchRadius < 1) g_securityGuardConfig.walkableSearchRadius = 1;
            g_securityGuardConfigLoaded = true;
        }
        return g_securityGuardConfig;
    }
}

namespace Game {

    bool SecurityGuard::seeded = false;

    /*!
     * \brief [AUTO] Initialize
     */
    void SecurityGuard::Initialize() {
        if (!seeded) {
            std::srand((unsigned)std::time(nullptr));
            seeded = true;
        }

        transform = gameObject->transform;
        BuildPathBFS_PatrolExplore();
    }

    /*!
     * \brief [AUTO] OnCCTVSpotted
     * \param worldPos [in]
     */
    /*!
     * \brief [AUTO] OnCCTVSpotted
     * \param worldPos [in]
     */
    /*!
     * \brief [AUTO] OnCCTVSpotted
     * \param worldPos [in]
     */
    /*!
     * \brief [AUTO] OnCCTVSpotted
     * \param worldPos [in]
     */
    /*!
     * \brief [AUTO] OnCCTVSpotted
     * \param worldPos [in]
     */
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

    void SecurityGuard::OnDistracted(const Engine::Vector2 &noisePos)
    {
        isDistracted = true;
        distractionPos = noisePos;

        memory.hasLastSeen = true;
        memory.lastSeenPos = noisePos;
        memory.timeSinceSeen = 0.0f;

        int targetX = static_cast<int>(noisePos.x);
        int targetY = static_cast<int>(noisePos.y);
        BuildPathBFS_ToGoal(targetX, targetY);

        SetState(GuardState::Search);
        searchTimer = 0.0f;
        repathTimer = 0.0f;
    }

    void SecurityGuard::ClearDistraction()
    {
        isDistracted = false;
    }

    /*!
     * \brief [AUTO] Update
     */
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
            break;
        }
    }

    /*!
     * \brief [AUTO] SetState
     * \param newState [in]
     */
    /*!
     * \brief [AUTO] SetState
     * \param newState [in]
     */
    /*!
     * \brief [AUTO] SetState
     * \param newState [in]
     */
    /*!
     * \brief [AUTO] SetState
     * \param newState [in]
     */
    /*!
     * \brief [AUTO] SetState
     * \param newState [in]
     */
    void SecurityGuard::SetState(GuardState newState) {
        if (state == newState) return;
        GuardState old = state;
        state = newState;

        // Start/stop looping chasing SFX when entering/exiting Chase
        if (newState == GuardState::Chase && old != GuardState::Chase) {
            // Avoid starting chase audio while the game is paused or player already caught
            if (!Engine::Application::IsPaused()) {
                if (playerData && playerData->IsCaught()) {
                    // Player already caught; do not start chase audio
                } else {
                    Engine::SoundManager::PlayLoop("guard_chasing", GetSecurityGuardConfig().chaseLoopVolume);
                }
            }
        }
        else if (old == GuardState::Chase && newState != GuardState::Chase) {
            Engine::SoundManager::StopLoop("guard_chasing");
        }
    }

    /*!
     * \brief [AUTO] StateToString
     * \param s [in]
     * \return 
     */
    /*!
     * \brief [AUTO] StateToString
     * \param s [in]
     * \return 
     */
    /*!
     * \brief [AUTO] StateToString
     * \param s [in]
     * \return 
     */
    /*!
     * \brief [AUTO] StateToString
     * \param s [in]
     * \return 
     */
    /*!
     * \brief [AUTO] StateToString
     * \param s [in]
     * \return 
     */
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

    /*!
     * \brief [AUTO] ChaseUpdate
     */
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
            repathTimer = GetSecurityGuardConfig().chaseRepathInterval;
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

    void SecurityGuard::SearchUpdate()
    {
        if (isDistracted)
        {
            repathTimer -= Engine::Time::deltaTime;
            if (repathTimer <= 0.0f)
            {
                BuildPathBFS_ToGoal((int)distractionPos.x, (int)distractionPos.y);
                repathTimer = GetSecurityGuardConfig().chaseRepathInterval;
            }

            if (path.empty() || pathIndex >= (int)path.size())
            {
                float dx = distractionPos.x - transform->position.x;
                float dy = distractionPos.y - transform->position.y;
                float distSq = dx * dx + dy * dy;

                auto rb = gameObject->GetComponent<Engine::Rigidbody2D>();
                if (distSq > 0.05f)
                {
                    float len = std::sqrt(distSq);
                    if (rb)
                        rb->velocity = Engine::Vector2((dx / len) * searchSpeed, (dy / len) * searchSpeed);
                }
                else
                {
                    if (rb)
                        rb->velocity = Engine::Vector2(0.0f, 0.0f);
                }
            }
            else
            {
                FollowPath(searchSpeed);
            }
            return;
        }

        if (investigatingSabotage)
        {
            if (IsCaught())
            {
                investigatingSabotage = false;
                SetState(GuardState::Catch);
                if (playerData)
                    playerData->Catch();
                return;
            }
        }
        else if (CanSeePlayer())
        {
            memory.lastSeenPos = {playerTf->position.x, playerTf->position.y};
            memory.hasLastSeen = true;
            memory.timeSinceSeen = 0.0f;
            repathTimer = 0.0f;
            cctvPursuitTimer = 0.0f;
            SetState(GuardState::Chase);
            return;
        }

        searchTimer += Engine::Time::deltaTime;

        if (investigatingSabotage && cctvTerminals)
        {
            sabotageInvestigateTimer -= Engine::Time::deltaTime;

            bool atDestination = (path.empty() || pathIndex >= (int)path.size());

            if (atDestination)
            {
                bool foundTerminal = false;
                for (auto *terminal : *cctvTerminals)
                {
                    if (!terminal || !terminal->isDisabled)
                        continue;

                    Engine::Vector2 termPos(terminal->transform->position.x, terminal->transform->position.y);
                    Engine::Vector2 guardPos(transform->position.x, transform->position.y);

                    float dx = termPos.x - guardPos.x;
                    float dy = termPos.y - guardPos.y;
                    float dist = std::sqrt(dx * dx + dy * dy);

                    if (dist <= GetSecurityGuardConfig().terminalRepairDistance && !terminal->guardIsRepairing)
                    {
                        terminal->StartRepair(this);
                        foundTerminal = true;
                        break;
                    }
                }

                if (!foundTerminal)
                {
                    int r = GetSecurityGuardConfig().fallbackSearchRadius;
                    int gx = (int)transform->position.x + (std::rand() % (r * 2 + 1) - r);
                    int gy = (int)transform->position.y + (std::rand() % (r * 2 + 1) - r);
                    if (tileMap && tileMap->IsWalkable((float)gx + 0.5f, (float)gy + 0.5f))
                    {
                        BuildPathBFS_ToGoal(gx, gy);
                    }
                }
            }

            if (sabotageInvestigateTimer <= 0.0f)
            {
                investigatingSabotage = false;
                sabotageInvestigateTimer = 0.0f;
                BuildPathBFS_PatrolExplore();
                SetState(GuardState::Patrol);
                return;
            }
        }

        if (!path.empty() && pathIndex < (int)path.size())
        {
            FollowPath(searchSpeed);
        }
        else
        {
            float oldMin = patrolMinGoalDist;
            patrolMinGoalDist = 2.0f;
            BuildPathBFS_PatrolExplore();
            patrolMinGoalDist = oldMin;
        }

        if (searchTimer >= searchDuration)
        {
            memory.hasLastSeen = false;
            memory.timeSinceSeen = 0.0f;
            investigatingSabotage = false;
            BuildPathBFS_PatrolExplore();
            SetState(GuardState::Patrol);
        }
    }

    /*!
     * \brief [AUTO] CanSeePlayer
     * \return 
     */
    /*!
     * \brief [AUTO] CanSeePlayer
     * \return 
     */
    /*!
     * \brief [AUTO] CanSeePlayer
     * \return 
     */
    /*!
     * \brief [AUTO] CanSeePlayer
     * \return 
     */
    /*!
     * \brief [AUTO] CanSeePlayer
     * \return 
     */
    bool SecurityGuard::CanSeePlayer() const {

        if (playerTf) {
            auto powerUpCtrl = playerTf->gameObject->GetComponent<PlayerPowerUpController>();
            if (powerUpCtrl && powerUpCtrl->HasPowerUp(PowerUpType::Invisibility)) {
                return false; // Guard immediately fails to see player
            }
        }

        float dx = playerTf->position.x - gameObject->transform->position.x;
        float dy = playerTf->position.y - gameObject->transform->position.y;
        return (dx * dx + dy * dy) <= detectRadius * detectRadius;
    }

    /*!
     * \brief [AUTO] IsCaught
     * \return 
     */
    /*!
     * \brief [AUTO] IsCaught
     * \return 
     */
    /*!
     * \brief [AUTO] IsCaught
     * \return 
     */
    /*!
     * \brief [AUTO] IsCaught
     * \return 
     */
    /*!
     * \brief [AUTO] IsCaught
     * \return 
     */
    bool SecurityGuard::IsCaught() const {

        if (playerData && playerData->IsInvulnerable()) {
            return false;
        }

        float dx = playerTf->position.x - gameObject->transform->position.x;
        float dy = playerTf->position.y - gameObject->transform->position.y;
        return (dx * dx + dy * dy) <= catchRadius * catchRadius;
    }

    /*!
     * \brief [AUTO] DistSqTo
     * \param x [in]
     * \param y [in]
     * \return 
     */
    /*!
     * \brief [AUTO] DistSqTo
     * \param x [in]
     * \param y [in]
     * \return 
     */
    /*!
     * \brief [AUTO] DistSqTo
     * \param x [in]
     * \param y [in]
     * \return 
     */
    /*!
     * \brief [AUTO] DistSqTo
     * \param x [in]
     * \param y [in]
     * \return 
     */
    /*!
     * \brief [AUTO] DistSqTo
     * \param x [in]
     * \param y [in]
     * \return 
     */
    float SecurityGuard::DistSqTo(float x, float y) const {
        float dx = x - gameObject->transform->position.x;
        float dy = y - gameObject->transform->position.y;
        return dx * dx + dy * dy;
    }

    /*!
     * \brief [AUTO] GetCurrentTile
     * \param tx [in]
     * \param ty [in]
     */
    /*!
     * \brief [AUTO] GetCurrentTile
     * \param tx [in]
     * \param ty [in]
     */
    /*!
     * \brief [AUTO] GetCurrentTile
     * \param tx [in]
     * \param ty [in]
     */
    /*!
     * \brief [AUTO] GetCurrentTile
     * \param tx [in]
     * \param ty [in]
     */
    /*!
     * \brief [AUTO] GetCurrentTile
     * \param tx [in]
     * \param ty [in]
     */
    void SecurityGuard::GetCurrentTile(int& tx, int& ty) const {
        tx = (int)gameObject->transform->position.x;
        ty = (int)gameObject->transform->position.y;
        tx = std::clamp(tx, 0, tileMap->width - 1);
        ty = std::clamp(ty, 0, tileMap->height - 1);
    }

    /*!
     * \brief [AUTO] FindDoorAtTile
     * \param tx [in]
     * \param ty [in]
     * \return 
     */
    /*!
     * \brief [AUTO] FindDoorAtTile
     * \param tx [in]
     * \param ty [in]
     * \return 
     */
    /*!
     * \brief [AUTO] FindDoorAtTile
     * \param tx [in]
     * \param ty [in]
     * \return 
     */
    /*!
     * \brief [AUTO] FindDoorAtTile
     * \param tx [in]
     * \param ty [in]
     * \return 
     */
    /*!
     * \brief [AUTO] FindDoorAtTile
     * \param tx [in]
     * \param ty [in]
     * \return 
     */
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

    /*!
     * \brief [AUTO] UpdateDoorAuto
     */
    void SecurityGuard::UpdateDoorAuto() {
        const float releaseDistSq = doorHoldReleaseDist * doorHoldReleaseDist;
        const float interactDist = GetSecurityGuardConfig().doorInteractDistance;
        const float interactDistSq = interactDist * interactDist;

        bool shouldReleaseHeldDoor = false;

        if (heldDoor) {
            int doorTx = heldDoor->tileX;
            int doorTy = heldDoor->tileY;

            bool stillTargetingHeldDoor = false;
            if (pathIndex < (int)path.size()) {
                int nextTx = (int)path[pathIndex].x;
                int nextTy = (int)path[pathIndex].y;
                stillTargetingHeldDoor = (nextTx == doorTx && nextTy == doorTy);
            }

            float doorCx = (float)doorTx + 0.5f;
            float doorCy = (float)doorTy + 0.5f;
            bool movedClearOfDoor = DistSqTo(doorCx, doorCy) >= releaseDistSq;

            shouldReleaseHeldDoor = (!stillTargetingHeldDoor && movedClearOfDoor);
        }

        Engine::Door* desiredDoor = nullptr;

        // Primary: door on the next waypoint tile.
        if (pathIndex < (int)path.size()) {
            int nextTx = (int)path[pathIndex].x;
            int nextTy = (int)path[pathIndex].y;
            desiredDoor = FindDoorAtTile(nextTx, nextTy);
        }

        // If we're currently on a door tile, keep that door open too.
        if (!desiredDoor) {
            int curTx = 0, curTy = 0;
            GetCurrentTile(curTx, curTy);
            desiredDoor = FindDoorAtTile(curTx, curTy);
        }

        // Recovery: if we just stepped off a door waypoint but got pushed back,
        // re-acquire that previous door until we've moved clear.
        if (!desiredDoor && pathIndex > 0) {
            int prevTx = (int)path[pathIndex - 1].x;
            int prevTy = (int)path[pathIndex - 1].y;
            Engine::Door* prevDoor = FindDoorAtTile(prevTx, prevTy);
            if (prevDoor) {
                float prevDoorCx = (float)prevTx + 0.5f;
                float prevDoorCy = (float)prevTy + 0.5f;
                bool stillNearPrevDoor = DistSqTo(prevDoorCx, prevDoorCy) < releaseDistSq;
                if (stillNearPrevDoor) {
                    desiredDoor = prevDoor;
                }
            }
        }

        // Constrained fallback for collision push-outs:
        // if path says move to next waypoint and no route door was found,
        // pick a nearby door only when it is also close to that next waypoint.
        if (!desiredDoor && pathIndex < (int)path.size()) {
            auto* scene = Engine::SceneManager::GetActiveScene();
            if (scene) {
                float bestD2 = interactDistSq + 1.0f;
                const Engine::Vector2 nextWp = path[pathIndex];
                for (auto& obj : scene->GetGameObjects()) {
                    if (!obj || !obj->active) continue;
                    auto* door = obj->GetComponent<Engine::Door>();
                    if (!door) continue;

                    float doorCx = (float)door->tileX + 0.5f;
                    float doorCy = (float)door->tileY + 0.5f;

                    float toGuardD2 = DistSqTo(doorCx, doorCy);
                    if (toGuardD2 > interactDistSq) continue;

                    float dxWp = nextWp.x - doorCx;
                    float dyWp = nextWp.y - doorCy;
                    float toWaypointD2 = dxWp * dxWp + dyWp * dyWp;
                    if (toWaypointD2 > 2.25f) continue; // ~1.5 tiles from next waypoint

                    if (toGuardD2 < bestD2) {
                        bestD2 = toGuardD2;
                        desiredDoor = door;
                    }
                }
            }
        }

        // If logic would release the held door but we still desire that same door,
        // keep holding it to avoid close/open churn and deadlock.
        if (heldDoor && desiredDoor == heldDoor) {
            shouldReleaseHeldDoor = false;
        }

        if (!desiredDoor) {
            if (heldDoor && shouldReleaseHeldDoor) {
                heldDoor->ReleaseOpen(this);
                heldDoor = nullptr;
            }
            return;
        }

        // Avoid rapid A<->B door switching while still near the currently held door.
        if (heldDoor && desiredDoor != heldDoor) {
            float heldDoorCx = (float)heldDoor->tileX + 0.5f;
            float heldDoorCy = (float)heldDoor->tileY + 0.5f;
            bool stillNearHeldDoor = DistSqTo(heldDoorCx, heldDoorCy) < releaseDistSq;
            if (stillNearHeldDoor) {
                desiredDoor = heldDoor;
                shouldReleaseHeldDoor = false;
            }
        }

        if (heldDoor != desiredDoor) {
            if (heldDoor) {
                heldDoor->ReleaseOpen(this);
            }
            heldDoor = desiredDoor;
        }

        // Reassert every frame so a forced close while held is immediately recovered.
        heldDoor->RequestOpen(this);
    }

    /*!
     * \brief [AUTO] FollowPath
     * \param speed [in]
     */
    /*!
     * \brief [AUTO] FollowPath
     * \param speed [in]
     */
    /*!
     * \brief [AUTO] FollowPath
     * \param speed [in]
     */
    /*!
     * \brief [AUTO] FollowPath
     * \param speed [in]
     */
    /*!
     * \brief [AUTO] FollowPath
     * \param speed [in]
     */
    void SecurityGuard::FollowPath(float speed) {
        auto rb = gameObject->GetComponent<Engine::Rigidbody2D>();

        if (pathIndex >= (int)path.size()) {
            if (rb) rb->velocity = Engine::Vector2(0.0f, 0.0f);
            return;
        }

        UpdateDoorAuto();

        // If we're holding a door open for us and it's not yet passable,
        // stop movement instead of pushing into the door collider which can
        // cause the guard to get stuck. We'll resume once the door opens.
        if (heldDoor && !heldDoor->IsPassable()) {
            if (rb) rb->velocity = Engine::Vector2(0.0f, 0.0f);
            return;
        }

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

    /*!
     * \brief [AUTO] BuildPathBFS_ToGoal
     * \param gx [in]
     * \param gy [in]
     */
    /*!
     * \brief [AUTO] BuildPathBFS_ToGoal
     * \param gx [in]
     * \param gy [in]
     */
    /*!
     * \brief [AUTO] BuildPathBFS_ToGoal
     * \param gx [in]
     * \param gy [in]
     */
    /*!
     * \brief [AUTO] BuildPathBFS_ToGoal
     * \param gx [in]
     * \param gy [in]
     */
    /*!
     * \brief [AUTO] BuildPathBFS_ToGoal
     * \param gx [in]
     * \param gy [in]
     */
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

    /*!
     * \brief [AUTO] BuildPathBFS_PatrolExplore
     */
    void SecurityGuard::BuildPathBFS_PatrolExplore() {
        if (!tileMap) return;

        int bestX = -1, bestY = -1;
        int bestScore = 1000000000;

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

    void SecurityGuard::OnCCTVSystemSabotaged(const Engine::Vector2& controlRoomPos, [[maybe_unused]] int sabotagedZoneId)
    {
        // Do not respond if actively catching the player
        if (state == GuardState::Catch) {
            return;
        }

        // Allow response from Patrol, Chase, Search � interrupt other behaviors to investigate

        // Trigger investigation behavior:
        memory.hasLastSeen = true;
        memory.lastSeenPos = controlRoomPos;
        memory.timeSinceSeen = 0.0f;
        investigatingSabotage = true;
        sabotageLocation = controlRoomPos;
        sabotageInvestigateTimer = GetSecurityGuardConfig().sabotageInvestigateDuration; // Investigate duration

        // Path to control room (add small offset to avoid wall placement)
        int targetX = (int)(controlRoomPos.x + 0.5f);
        int targetY = (int)(controlRoomPos.y + 0.5f);
        
        // Ensure target is walkable
        if (tileMap && !tileMap->IsWalkable((float)targetX + 0.5f, (float)targetY + 0.5f)) {
            // Find nearest walkable tile
            for (int dy = -GetSecurityGuardConfig().walkableSearchRadius; dy <= GetSecurityGuardConfig().walkableSearchRadius; ++dy) {
                for (int dx = -GetSecurityGuardConfig().walkableSearchRadius; dx <= GetSecurityGuardConfig().walkableSearchRadius; ++dx) {
                    int testX = targetX + dx;
                    int testY = targetY + dy;
                    if (testX < 0 || testY < 0 || testX >= tileMap->width || testY >= tileMap->height) continue;
                    if (tileMap->IsWalkable((float)testX + 0.5f, (float)testY + 0.5f)) {
                        targetX = testX;
                        targetY = testY;
                        goto foundWalkable;
                    }
                }
            }
            foundWalkable:;
        }
        
        BuildPathBFS_ToGoal(targetX, targetY);

        // Switch to Search state to investigate (will use SearchUpdate to start repair when arrived)
        SetState(GuardState::Search);
        searchTimer = 0.0f;
    }

} // namespace Game
