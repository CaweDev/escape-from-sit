#pragma once
#include "Engine/Core/Component.hpp"
#include <unordered_map>

namespace Game {

    class Player; // Forward declaration

    // Universal PowerUp definition
    enum class PowerUpType {
        SpeedBoost = 0,
        InfiniteStamina,
        Shield,
        Invisibility
    };

    /**
     * Component that tracks active timers for buffs natively on the player
     */
    class PlayerPowerUpController : public Engine::Component {
    public:
        Player* player = nullptr;
        std::unordered_map<PowerUpType, float> activeTimers;

        void Awake() override;
        void Update() override;

        // Called by items when the player walks over them/interacts
        void AddPowerUp(PowerUpType type, float duration);
        void RemovePowerUp(PowerUpType type);
        bool HasPowerUp(PowerUpType type) const {
             return activeTimers.find(type) != activeTimers.end();
		}

    private:
        // Safely toggles Player flags
        void UpdatePlayerFlags(PowerUpType type, bool isActive);
       
    };

} // namespace Game