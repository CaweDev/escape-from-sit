#include "CameraShake.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Core/Time.hpp"
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
        m_timeLeft = 0.0f;
        m_duration = 0.0f;
        m_intensity = 0.0f;
        m_basePitch = 0.0f;
    }

    void CameraShake::Shake(float duration, float intensity)
    {
        // Get current camera pitch as baseline
        auto cam = gameObject->GetComponent<Engine::Camera>();
        if (cam) {
            m_basePitch = cam->pitch;
        }

        m_duration = duration;
        m_timeLeft = duration;
        m_intensity = intensity;
    }

    void CameraShake::Update()
    {
        float dt = Engine::Time::deltaTime;

        // Get camera component
        auto cam = gameObject->GetComponent<Engine::Camera>();
        if (!cam) return;

        // IMPORTANT: when not shaking, reset to base
        if (m_timeLeft <= 0.0f)
        {
            // No shake - camera pitch stays at base
            return;
        }

        m_timeLeft -= dt;

        // Calculate shake intensity (fades out)
        float t = (m_duration > 0.0f) ? (m_timeLeft / m_duration) : 0.0f;
        float scale = (t < 0.0f) ? 0.0f : t;

        // Apply shake to camera pitch ONLY (visual offset, doesn't affect movement)
        float shakeOffset = RandNeg1To1() * m_intensity * scale * 100.0f; // Multiply by 100 for visible effect
        cam->pitch = m_basePitch + shakeOffset;

        // Optionally add small rotation shake
        float rotShake = RandNeg1To1() * m_intensity * scale * 2.0f;
        transform->rotation += rotShake;

        // When shake ends, reset
        if (m_timeLeft <= 0.0f)
        {
            cam->pitch = m_basePitch;
        }
    }
}