#pragma once
#include "Engine/Core/Component.hpp"
#include "Engine/Math/Vector2.hpp"

namespace Game {

    class CameraShake : public Engine::Component
    {
    public:
        void Initialize() override;
        void Update() override;

        void Shake(float duration, float intensity);
        // Short pronounced pulse for "pounding headache" effect.
        void Pulse(float duration, float intensity);

    private:
        float m_lastShakeOffset = 0.0f;
        float m_duration = 0.0f;
        float m_timeLeft = 0.0f;
        float m_intensity = 0.0f;
        // Pulse state (for pounding/throbbing)
        float m_pulseDuration = 0.0f;
        float m_pulseTimeLeft = 0.0f;
        float m_pulseIntensity = 0.0f;
        float m_lastPulseOffset = 0.0f;
    };

}