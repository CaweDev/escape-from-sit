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

    private:
        Engine::Vector2 m_basePos2D{};
        float m_duration = 0.0f;
        float m_timeLeft = 0.0f;
        float m_intensity = 0.0f;
    };

}