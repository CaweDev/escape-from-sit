/*!
 * \file UCB1Bandit.hpp
 * \brief Header for a simple UCB1 multi-armed bandit implementation
 */

#pragma once
#include <vector>

namespace Game {

    /*!
     * \class UCB1Bandit
     * \brief Implementation of the UCB1 algorithm for exploration/exploitation.
     */
    class UCB1Bandit {
    public:
        /*! \brief Construct bandit with given number of arms and exploration factor c. */
        UCB1Bandit(int arms = 0, float c = 1.0f);

        /*! \brief Reset internal state to given arms and c. */
        void Reset(int arms, float c);

        /*! \brief Select an arm index [0..arms-1] according to UCB1. \return chosen arm or -1 if none */
        int  SelectArm();

        /*! \brief Update statistics for a given arm with reward in [0,1]. */
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
