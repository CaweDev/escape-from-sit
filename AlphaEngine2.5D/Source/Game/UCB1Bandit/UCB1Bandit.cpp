/*!
 * \file AlphaEngine2.5D\Source\Game\UCB1Bandit\UCB1Bandit.cpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief [AUTO] Added Doxygen file header
 */

#include "UCB1Bandit.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <cmath>
#include <limits>
#include <algorithm> // std::max, std::clamp

namespace
{
    struct UCB1BanditConfig : public Engine::IConfig
    {
        int minArms = 0;
        int minTotalPullsForLog = 1;
        float rewardMin = 0.0f;
        float rewardMax = 1.0f;

        void FromJson(const Json::Value& j) override
        {
            minArms = j.get("minArms", minArms).asInt();
            minTotalPullsForLog = j.get("minTotalPullsForLog", minTotalPullsForLog).asInt();
            rewardMin = j.get("rewardMin", rewardMin).asFloat();
            rewardMax = j.get("rewardMax", rewardMax).asFloat();
        }
    };

    UCB1BanditConfig g_ucb1BanditConfig;
    bool g_ucb1BanditConfigLoaded = false;

    const UCB1BanditConfig& GetUCB1BanditConfig()
    {
        if (!g_ucb1BanditConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/ucb1_bandit.json", g_ucb1BanditConfig);
            if (g_ucb1BanditConfig.minArms < 0) g_ucb1BanditConfig.minArms = 0;
            if (g_ucb1BanditConfig.minTotalPullsForLog < 1) g_ucb1BanditConfig.minTotalPullsForLog = 1;
            if (g_ucb1BanditConfig.rewardMin > g_ucb1BanditConfig.rewardMax)
            {
                float t = g_ucb1BanditConfig.rewardMin;
                g_ucb1BanditConfig.rewardMin = g_ucb1BanditConfig.rewardMax;
                g_ucb1BanditConfig.rewardMax = t;
            }
            g_ucb1BanditConfigLoaded = true;
        }
        return g_ucb1BanditConfig;
    }
}

namespace Game {

    UCB1Bandit::UCB1Bandit(int arms, float c)
    {
        Reset(arms, c);
    }

    void UCB1Bandit::Reset(int arms, float c)
    {
        const auto& cfg = GetUCB1BanditConfig();
        if (arms < cfg.minArms) arms = cfg.minArms;
        m_c = c;
        m_totalPulls = 0;
        m_pulls.assign((size_t)arms, 0);
        m_values.assign((size_t)arms, 0.0f);
    }

    int UCB1Bandit::SelectArm()
    {
        const int k = (int)m_pulls.size();
        if (k == 0) return -1;

        // Ensure each arm is tried at least once
        for (int i = 0; i < k; ++i)
            if (m_pulls[i] == 0)
                return i;

        // UCB = mean + c * sqrt( ln(N) / n_i )
        const auto& cfg = GetUCB1BanditConfig();
        const float lnN = std::log((float)std::max(cfg.minTotalPullsForLog, m_totalPulls));

        int best = 0;
        float bestScore = -std::numeric_limits<float>::infinity();

        for (int i = 0; i < k; ++i)
        {
            const float mean = m_values[i];
            const float bonus = m_c * std::sqrt(lnN / (float)m_pulls[i]);
            const float score = mean + bonus;

            if (score > bestScore)
            {
                bestScore = score;
                best = i;
            }
        }

        return best;
    }

    void UCB1Bandit::Update(int arm, float reward)
    {
        const int k = (int)m_pulls.size();
        if (arm < 0 || arm >= k) return;

        const auto& cfg = GetUCB1BanditConfig();
        reward = std::clamp(reward, cfg.rewardMin, cfg.rewardMax);

        m_totalPulls += 1;
        m_pulls[arm] += 1;

        // Incremental mean update
        const float n = (float)m_pulls[arm];
        const float old = m_values[arm];
        m_values[arm] = old + (reward - old) / n;
    }

} // namespace Game