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

    void CameraShake::Update()
    {
        float dt = Engine::Time::deltaTime;

        // Get camera component
        auto cam = gameObject->GetComponent<Engine::Camera>();
        if (!cam) return;

        // Remove previous shake offset from camera pitch
        cam->pitch -= m_lastShakeOffset;

        // If shake is finished, clear offset and return
        if (m_timeLeft <= 0.0f)
        {
            m_lastShakeOffset = 0.0f;
            return;
        }

        m_timeLeft -= dt;

        // Calculate shake intensity (fades out)
        float t = (m_duration > 0.0f) ? (m_timeLeft / m_duration) : 0.0f;
        float scale = (t < 0.0f) ? 0.0f : t;

        // Calculate NEW shake offset (doesn't overwrite player's pitch input)
        m_lastShakeOffset = RandNeg1To1() * m_intensity * scale * 100.0f;
        
        // ADD shake offset to current pitch (preserves player input)
        cam->pitch += m_lastShakeOffset;

        // Optionally add small rotation shake
        float rotShake = RandNeg1To1() * m_intensity * scale * 2.0f;
        transform->rotation += rotShake;
    }
}