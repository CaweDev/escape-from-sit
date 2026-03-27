/**
 * @file UCB1Bandit.hpp
 * @description Header for the UCB1 Multi-Armed Bandit algorithm implementation.
 */
#pragma once
#include <vector>

namespace Game {

    /**
     * @class UCB1Bandit
     * @description Implementation of the UCB1 (Upper Confidence Bound) algorithm for spatial optimization.
     */
    class UCB1Bandit {
    public:
        UCB1Bandit(int arms = 0, float c = 1.0f);

        void Reset(int arms, float c);

        // returns arm index in [0..arms-1], or -1 if no arms
        int  SelectArm();

        // reward is clamped to [0,1]
        void Update(int arm, float reward);

        int  GetArms() const { return (int)m_pulls.size(); }
        int  GetTotalPulls() const { return m_totalPulls; }

        // Debug accessors
        int   Pulls(int arm) const {
            if (arm < 0 || arm >= (int)m_pulls.size()) return 0;
            return m_pulls[arm];
        }
        float Value(int arm) const {
            if (arm < 0 || arm >= (int)m_values.size()) return 0.0f;
            return m_values[arm];
        }

    private:
        float m_c = 1.0f;
        int   m_totalPulls = 0;
        std::vector<int>   m_pulls;
        std::vector<float> m_values; // mean reward per arm
    };

} // namespace Game