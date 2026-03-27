/**
 * @file PlayerPowerUpController.hpp
 * @brief Header for the PlayerPowerUpController component that tracks active buffs on the player.
 */
#pragma once

#include "Engine/Core/Component.hpp"
#include <unordered_map>

namespace Game {

    class Player; // Forward declaration

    /**
     * @enum PowerUpType
     * @brief Types of power-ups available in the game.
     */
    enum class PowerUpType {
        SpeedBoost = 0,     ///< Increases player movement speed.
        InfiniteStamina,    ///< Prevents stamina depletion.
        Shield,             ///< Protects player from a single hit.
        Invisibility        ///< Makes player invisible to NPCs and cameras.
    };

    /**
     * @class PlayerPowerUpController
     * @brief Component that tracks active timers for buffs natively on the player.
     */
    class PlayerPowerUpController : public Engine::Component {
    public:
        Player* player = nullptr;                       ///< Reference to the player data.
        std::unordered_map<PowerUpType, float> activeTimers; ///< Mapping of active power-up types to remaining time.

        /**
         * @brief Called when the component is created.
         */
        void Awake() override;

        /**
         * @brief Updates active timers and applies/removes effects.
         */
        void Update() override;

        /**
         * @brief Adds a power-up effect to the player.
         * @param type The type of power-up to add.
         * @param duration The duration of the effect in seconds.
         */
        void AddPowerUp(PowerUpType type, float duration);

        /**
         * @brief Removes a power-up effect from the player.
         * @param type The type of power-up to remove.
         */
        void RemovePowerUp(PowerUpType type);

        /**
         * @brief Checks if the player currently has an active power-up of a specific type.
         * @param type The type of power-up to check.
         * @return bool True if the power-up is active.
         */
        bool HasPowerUp(PowerUpType type) const {
             return activeTimers.find(type) != activeTimers.end();
		}

    private:
        /**
         * @brief Safely toggles Player flags based on power-up activity.
         * @param type The type of power-up.
         * @param isActive True if the effect is being applied, false if removed.
         */
        void UpdatePlayerFlags(PowerUpType type, bool isActive);
    };

} // namespace Game
