#include "CameraShake.hpp"
#include "Engine/Core/Transform.hpp"
#include <cstdlib>
#include <cmath>

namespace Game
{
    static float RandNeg1To1()
    {
        return ((float)std::rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    }

    void CameraShake::Initialize()
    {
        // start base at current
        m_basePos2D.x = transform->position.x;
        m_basePos2D.y = transform->position.y;
        m_timeLeft = 0.0f;
        m_duration = 0.0f;
        m_intensity = 0.0f;
    }

    void CameraShake::Shake(float duration, float intensity)
    {
        // base should be "where the player is NOW"
        m_basePos2D.x = transform->position.x;
        m_basePos2D.y = transform->position.y;

        m_duration = duration;
        m_timeLeft = duration;
        m_intensity = intensity;
    }

    void CameraShake::Update()
    {
        float dt = 0.016f; // replace with Engine::Time::deltaTime if available

        // IMPORTANT: when not shaking, DO NOT override movement
        if (m_timeLeft <= 0.0f)
        {
            // keep base following the real position
            m_basePos2D.x = transform->position.x;
            m_basePos2D.y = transform->position.y;
            return;
        }

        m_timeLeft -= dt;

        float t = (m_duration > 0.0f) ? (m_timeLeft / m_duration) : 0.0f;
        float scale = (t < 0.0f) ? 0.0f : t; // fade out

        float ox = RandNeg1To1() * m_intensity * scale;
        float oy = RandNeg1To1() * m_intensity * scale;

        transform->position.x = m_basePos2D.x + ox;
        transform->position.y = m_basePos2D.y + oy;

        // end: snap back once
        if (m_timeLeft <= 0.0f)
        {
            transform->position.x = m_basePos2D.x;
            transform->position.y = m_basePos2D.y;
        }
    }
}