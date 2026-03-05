// =======================
// UCB1Bandit.cpp
// =======================
#include "UCB1Bandit.hpp"
#include <cmath>
#include <limits>
#include <algorithm> // std::max, std::clamp

UCB1Bandit::UCB1Bandit(int arms, float c)
{
    Reset(arms, c);
}

void UCB1Bandit::Reset(int arms, float c)
{
    if (arms < 0) arms = 0;
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
    const float lnN = std::log((float)std::max(1, m_totalPulls));

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

    reward = std::clamp(reward, 0.0f, 1.0f);

    m_totalPulls += 1;
    m_pulls[arm] += 1;

    // Incremental mean update
    const float n = (float)m_pulls[arm];
    const float old = m_values[arm];
    m_values[arm] = old + (reward - old) / n;
}