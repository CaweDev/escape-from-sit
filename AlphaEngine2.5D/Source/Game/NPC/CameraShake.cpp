/*!
 * \file AlphaEngine2.5D\Source\Game\NPC\CameraShake.cpp
 * \brief Component that applies procedural camera shake effects
 */

#include "CameraShake.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <cstdlib>
#include <cmath>

namespace
{
    struct CameraShakeConfig : public Engine::IConfig
    {
        float randMin = -1.0f;
        float randMax = 1.0f;
        float shakePitchScale = 100.0f;
        float shakeRotationScale = 2.0f;
        float pulsePitchScale = 120.0f;
        float pulseWavePi = 3.14159265f;

        void FromJson(const Json::Value& j) override
        {
            randMin = j.get("randMin", randMin).asFloat();
            randMax = j.get("randMax", randMax).asFloat();
            shakePitchScale = j.get("shakePitchScale", shakePitchScale).asFloat();
            shakeRotationScale = j.get("shakeRotationScale", shakeRotationScale).asFloat();
            pulsePitchScale = j.get("pulsePitchScale", pulsePitchScale).asFloat();
            pulseWavePi = j.get("pulseWavePi", pulseWavePi).asFloat();
        }
    };

    CameraShakeConfig g_cameraShakeConfig;
    bool g_cameraShakeConfigLoaded = false;

    const CameraShakeConfig& GetCameraShakeConfig()
    {
        if (!g_cameraShakeConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/camera_shake.json", g_cameraShakeConfig);
            if (g_cameraShakeConfig.randMin > g_cameraShakeConfig.randMax)
            {
                float t = g_cameraShakeConfig.randMin;
                g_cameraShakeConfig.randMin = g_cameraShakeConfig.randMax;
                g_cameraShakeConfig.randMax = t;
            }
            g_cameraShakeConfigLoaded = true;
        }
        return g_cameraShakeConfig;
    }

    static float RandNeg1To1()
    {
        const auto& cfg = GetCameraShakeConfig();
        const float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        return cfg.randMin + t * (cfg.randMax - cfg.randMin);
    }
}

namespace Game
{
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
        const auto& cfg = GetCameraShakeConfig();
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
            m_lastShakeOffset = RandNeg1To1() * m_intensity * scale * cfg.shakePitchScale;
            cam->pitch += m_lastShakeOffset;
            float rotShake = RandNeg1To1() * m_intensity * scale * cfg.shakeRotationScale;
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
            float env = std::sin(pt * cfg.pulseWavePi);
            env = env * env; // sharpen

            m_lastPulseOffset = env * m_pulseIntensity * cfg.pulsePitchScale;
            cam->pitch += m_lastPulseOffset;
        }
    }
}