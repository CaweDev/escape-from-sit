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
        m_lastShakeOffset = 0.0f;
    }

    void CameraShake::Shake(float duration, float intensity)
    {
        m_duration = duration;
        m_timeLeft = duration;
        m_intensity = intensity;
    }

    void CameraShake::Pulse(float duration, float intensity)
    {
        // Pulse does not interrupt ongoing shake; it's additive and short-lived
        m_pulseDuration = duration;
        m_pulseTimeLeft = duration;
        m_pulseIntensity = intensity;
        m_lastPulseOffset = 0.0f;
    }

    void CameraShake::Update()
    {
        float dt = Engine::Time::deltaTime;

        // Get camera component
        auto cam = gameObject->GetComponent<Engine::Camera>();
        if (!cam) return;

        // Remove previous shake offset from camera pitch
        cam->pitch -= m_lastShakeOffset;

        // If shake is active, apply normal shake (continuous)
        if (m_timeLeft > 0.0f)
        {
            m_timeLeft -= dt;
            float t = (m_duration > 0.0f) ? (m_timeLeft / m_duration) : 0.0f;
            float scale = (t < 0.0f) ? 0.0f : t;
            m_lastShakeOffset = RandNeg1To1() * m_intensity * scale * 100.0f;
            cam->pitch += m_lastShakeOffset;
            float rotShake = RandNeg1To1() * m_intensity * scale * 2.0f;
            transform->rotation += rotShake;
        }

        // Handle pounding pulse (throbbing): short additive offset that decays quickly
        if (m_pulseTimeLeft > 0.0f)
        {
            // remove previous pulse offset
            cam->pitch -= m_lastPulseOffset;

            m_pulseTimeLeft -= dt;
            float pt = (m_pulseDuration > 0.0f) ? (m_pulseTimeLeft / m_pulseDuration) : 0.0f;
            if (pt < 0.0f) pt = 0.0f;

            // Use a non-linear envelope for a sharp thump: (sin^2) shape
            float env = std::sin(pt * 3.14159265f);
            env = env * env; // sharpen

            m_lastPulseOffset = env * m_pulseIntensity * 120.0f; // scale to pitch units
            cam->pitch += m_lastPulseOffset;
        }
    }
}