/**
 * @file PowerUpManager.hpp
 * @brief Header for the PowerUpManager system that handles spawning and initialization of power-ups.
 */
#pragma once

#include "PowerUp.hpp"
#include <vector>

namespace Game {

    /**
     * @class PowerUpManager
     * @brief Static manager for power-up data and object creation.
     */
    class PowerUpManager {
    public:
        static std::vector<PowerUpData> allPowerUps; ///< Master list of all power-up data.

        /**
         * @brief Spawns a PowerUpObject in the scene based on the provided data.
         * @param data The power-up data.
         */
        static void CreatePowerUpObject(PowerUpData data);

        /**
         * @brief Initializes static power-up data (hardcoded or from config).
         */
        static void PowerUpDataInit();

        /**
         * @brief Initializes the power-up manager and spawns objects.
         */
        static void Init();

        /**
         * @brief Cleans up manager resources.
         */
        static void FreeManager();
    };

} // namespace Game
