/*!
 * \file AlphaEngine2.5D\Source\Game\CCTV\CCTVCamera.cpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief [AUTO] Added Doxygen file header
 */

#include "CCTVCamera.hpp"
#include "Game/Scenes/FPSScene/UI/PlayerHUD.hpp"
#include "Game/Player/Player.hpp"
#include "Engine/UI/Specialized/Minimap.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Audio/SoundManager.hpp"
#include <cmath>
#include  "Engine/Core/GameObject.hpp"
#include "Engine/Core/ConfigManager.hpp"

/*!
 * \file CCTVCamera.cpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief Implementation of CCTV camera detection and visualization
 */

namespace {

    /*! \struct CCTVCameraConfig
     *  \brief JSON-driven configuration for CCTVCamera
     */
    struct CCTVCameraConfig : public Engine::IConfig
    {
        float degToRad = 3.14159265f / 180.0f;
        float epsilon = 0.0001f;
        Engine::Color disabledColor = Engine::Color(0.5f, 0.5f, 0.5f, 1.0f);
        Engine::Color detectingColor = Engine::Color(1.0f, 0.3f, 0.3f, 1.0f);
        Engine::Color idleColor = Engine::Color(1.0f, 1.0f, 1.0f, 1.0f);

        /*! \brief Parse color object from JSON with fallback. */
        static Engine::Color ParseColor(const Json::Value& j, const char* key, const Engine::Color& fallback)
        {
            if (!j.isMember(key) || !j[key].isObject()) return fallback;
            const auto& c = j[key];
            return Engine::Color(
                c.get("r", fallback.r).asFloat(),
                c.get("g", fallback.g).asFloat(),
                c.get("b", fallback.b).asFloat(),
                c.get("a", fallback.a).asFloat());
        }

        /*! \brief Populate config fields from JSON. */
        void FromJson(const Json::Value& j) override
        {
            degToRad = j.get("degToRad", degToRad).asFloat();
            epsilon = j.get("epsilon", epsilon).asFloat();
            disabledColor = ParseColor(j, "disabledColor", disabledColor);
            detectingColor = ParseColor(j, "detectingColor", detectingColor);
            idleColor = ParseColor(j, "idleColor", idleColor);
        }
    };

    CCTVCameraConfig g_cctvCamConfig;
    bool g_cctvCamConfigLoaded = false;

    const CCTVCameraConfig& GetCCTVCameraConfig()
    {
        if (!g_cctvCamConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/cctv_camera.json", g_cctvCamConfig);
            if (g_cctvCamConfig.epsilon <= 0.0f) g_cctvCamConfig.epsilon = 0.0001f;
            g_cctvCamConfigLoaded = true;
        }
        return g_cctvCamConfig;
    }

    /*! \brief Convert degrees to radians using configured multiplier. */
    static float DegToRad(float deg) { return deg * GetCCTVCameraConfig().degToRad; }

}

namespace Game {

    /*! \brief Test whether a direction vector falls inside the camera cone. \param dx [in] X delta \param dy [in] Y delta \return true if inside FOV */
    bool CCTVCamera::InFov(float dx, float dy) const
    {
        // Defensive: ensure transform exists
        if (!gameObject || !gameObject->transform) return false;

        // camera forward from rotation (assuming degrees, like your scene)
        float rotRad = DegToRad((float)gameObject->transform->rotation);
        Vector2 fwd(std::cos(rotRad), std::sin(rotRad));

        Vector2 to(dx, dy);
        float len = std::sqrt(to.x * to.x + to.y * to.y);
        if (len <= GetCCTVCameraConfig().epsilon) return true;
        to.x /= len; to.y /= len;

        float dot = fwd.x * to.x + fwd.y * to.y;
        float half = DegToRad(coneWidth * 0.5f);
        float cosHalf = std::cos(half);

        return dot >= cosHalf;
    }

    /*! \brief Raycast against tilemap to check line-of-sight between two points. \param from [in] Ray origin \param to [in] Ray target \return true if LOS is clear */
    bool CCTVCamera::RaycastTileLOS(Vector2 from, Vector2 to) const
    {
        if (!tile) return false;

        Vector2 dir = Vector2(to.x - from.x, to.y - from.y);
        float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (dist <= GetCCTVCameraConfig().epsilon) return true;

        dir.x /= dist;
        dir.y /= dist;

        // step along ray; stop if hits non-walkable tile
        float step = 1.0f / rayStepPerUnit;
        float t = 0.0f;

        while (t <= dist)
        {
            float px = from.x + dir.x * t;
            float py = from.y + dir.y * t;

            // If your TileMap has a better "IsWall" method, swap this out.
            // Using IsWalkable as "not a wall".
            if (!tile->IsWalkable(px, py))
                return false;

            t += step;
        }

        return true;
    }

    /*! \brief Wrapper to test LOS to the tracked player transform. \return true if LOS exists or LOS not required */
    bool CCTVCamera::HasLineOfSighttoPlayer() const
    {
        if (!tile || !tf) return false;
        if (!gameObject || !gameObject->transform) return false;

        Vector2 from(gameObject->transform->position.x, gameObject->transform->position.y);
        Vector2 to(tf->position.x, tf->position.y);

        if (!needLOS) return true;
        return RaycastTileLOS(from, to);
    }

    /*! \brief Record player's world/tile position as last seen. */
    void CCTVCamera::RecordLastSeen()
    {
        if (!tf) return;

        lastSeenWorld = Vector2(tf->position.x, tf->position.y);
        lastSeenTile = Vector2((float)(int)tf->position.x, (float)(int)tf->position.y);
        hasLastSeen = true;
    }

    /*! \brief Test if a given tile is visible by this camera. \param tx [in] Tile X \param ty [in] Tile Y \return true if tile visible */
    bool CCTVCamera::IsTileVisible(int tx, int ty) const
    {
        // basic check: treat tile center as point
        Vector2 camPos(gameObject->transform->position.x, gameObject->transform->position.y);
        Vector2 p((float)tx + 0.5f, (float)ty + 0.5f);

        float dx = p.x - camPos.x;
        float dy = p.y - camPos.y;

        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > maxRange) return false;
        if (dist > detectRadius) return false;

        if (!InFov(dx, dy)) return false;
        if (needLOS && !RaycastTileLOS(camPos, p)) return false;

        return true;
    }

    /*! \brief Raycast in a direction until a wall is hit or maxDist reached. \param from [in] Origin \param dir [in] Unit direction \param maxDist [in] Maximum distance \return Impact position */
    Engine::Vector2 CCTVCamera::RaycastToWall(Vector2 from, Vector2 dir, float maxDist) const
    {
        if (!tile) return Vector2(from.x + dir.x * maxDist, from.y + dir.y * maxDist);

        float step = 1.0f / rayStepPerUnit;
        float t = 0.0f;

        while (t <= maxDist)
        {
            float px = from.x + dir.x * t;
            float py = from.y + dir.y * t;

            if (!tile->IsWalkable(px, py))
                return Vector2(px, py);

            t += step;
        }

        return Vector2(from.x + dir.x * maxDist, from.y + dir.y * maxDist);
    }

    /*! \brief Build polygon points representing visible region (for minimap). */
    void CCTVCamera::BuildVisionPolygon()
    {
        m_visionPoly.clear();
        if (!showVision) return;
        if (!gameObject || !gameObject->transform) return;

        Vector2 camPos(gameObject->transform->position.x, gameObject->transform->position.y);

        float baseRot = DegToRad((float)gameObject->transform->rotation);
        float half = DegToRad(coneWidth * 0.5f);

        // fan out rays within cone
        for (int i = 0; i <= rayCount; ++i)
        {
            float t = (rayCount == 0) ? 0.0f : (float)i / (float)rayCount;
            float ang = (baseRot - half) + (2.0f * half) * t;

            Vector2 dir(std::cos(ang), std::sin(ang));
            Vector2 hit = RaycastToWall(camPos, dir, maxRange);
            m_visionPoly.push_back(hit);
        }
    }

    /*! \brief Main per-frame update: detection, sounds, minimap and sprite color. */
    void CCTVCamera::Update()
    {
       
        justSpotted = false;
        // Defensive: ensure essential pointers are valid
        if (!gameObject || !gameObject->transform || !tile || !tf)
        {
            isSeeingPlayer = false;
            justSpotted = false;
            return;
        }
        
        // Check if disabled by terminal
        if (isDisabled)
        {
            isSeeingPlayer = false;
            justSpotted = false;
            
            // Update disabled timer
            if (disabledTimeRemaining > 0.0f)
            {
                disabledTimeRemaining -= Engine::Time::deltaTime;
                
                if (disabledTimeRemaining <= 0.0f)
                {
                    isDisabled = false;
                    disabledTimeRemaining = 0.0f;
                    printf("[CCTV %d] Auto-restored after timeout\n", id);
                }
            }
            
            // Update sprite color for visual feedback
            if (spriteRenderer)
            {
                spriteRenderer->SetColor(GetCCTVCameraConfig().disabledColor);
            }
            else
            {
                printf("[CCTV %d] WARNING: spriteRenderer is NULL!\n", id);
            }
            
            return; // Don't run detection logic when disabled
        }

        BuildVisionPolygon();
        
        Vector2 camPos(gameObject->transform->position.x, gameObject->transform->position.y);
        Vector2 playerPos(tf->position.x, tf->position.y);

        float dx = playerPos.x - camPos.x;
        float dy = playerPos.y - camPos.y;

        float dist = std::sqrt(dx * dx + dy * dy);

        bool inRange = (dist <= maxRange);
        bool inDetectRange = (dist <= detectRadius);
        bool inFov = inRange && InFov(dx, dy);
        bool los = Player::isInvisible ? false : (inFov && HasLineOfSighttoPlayer());

        bool nowSeeing = (inDetectRange && inFov && los);

        if (nowSeeing && !isSeeingPlayer)
        {
            justSpotted = true;   // edge trigger
            // Debug output
            //printf("[CCTV %d] Player spotted at (%.1f, %.1f)!\n", id, playerPos.x, playerPos.y);
            
            // Trigger HUD warning
            if (hud) {
                hud->ShowCCTVWarning(true);
            }
            // Play CCTV detected SFX
            // Use PlayLoop for sustained alert (ensure it stops when player out of sight)
            // Do not start guard chase audio if game is paused or player already caught
            if (!Engine::Application::IsPaused()) {
                auto* playerComp = tf && tf->gameObject ? tf->gameObject->GetComponent<Game::Player>() : nullptr;
                const bool playerCaught = playerComp ? playerComp->IsCaught() : false;
                if (!playerCaught) {
                    Engine::SoundManager::PlayLoop("guard_chasing", 1.0f);
                }
            }
        }

        // Update sight state
        bool prevSeeing = isSeeingPlayer;
        isSeeingPlayer = nowSeeing;

        // If we lost the player, stop the alert loop
        if (prevSeeing && !isSeeingPlayer) {
            Engine::SoundManager::StopLoop("guard_chasing");
        }

        if (isSeeingPlayer)
        {
            RecordLastSeen();
        }
        
        // Update minimap vision cone
        if (minimap && gameObject && gameObject->transform)
        {
            minimap->UpdateCCTVCone(gameObject->transform, coneWidth, maxRange, isSeeingPlayer);
        }
        
        // Update sprite color for visual feedback in 3D world
        if (spriteRenderer)
        {
            if (isSeeingPlayer)
            {
                spriteRenderer->SetColor(GetCCTVCameraConfig().detectingColor);
            }
            else
            {
                spriteRenderer->SetColor(GetCCTVCameraConfig().idleColor);
            }
        }
    }

} // namespace Game