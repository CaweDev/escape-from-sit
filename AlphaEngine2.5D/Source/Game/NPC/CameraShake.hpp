/*!
 * \file CameraShake.hpp
 * \brief Component that applies procedural camera shake effects
 */

#pragma once
#include "Engine/Core/Component.hpp"
#include "Engine/Math/Vector2.hpp"

namespace Game {

    /*!
     * \class CameraShake
     * \brief Component applied to camera to produce shake and pulse effects
     */
    class CameraShake : public Engine::Component
    {
    public:
        /*! \brief Initialize component state. */
        void Initialize() override;
        /*! \brief Per-frame update to apply offsets. */
        void Update() override;

        /*! \brief Start continuous shake. \param duration [in] seconds \param intensity [in] magnitude */
        void Shake(float duration, float intensity);
        /*! \brief Short pronounced pulse for pounding effect. \param duration [in] seconds \param intensity [in] magnitude */
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
