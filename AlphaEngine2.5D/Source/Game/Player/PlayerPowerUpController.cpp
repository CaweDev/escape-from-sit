#include "PlayerPowerUpController.h"
#include "Game/Player/Player.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include <iostream>

namespace Game {

void PlayerPowerUpController::Awake() {
    player = gameObject->GetComponent<Player>();
}

void PlayerPowerUpController::Update() {
    if (!player) return;

    for (auto it = activeTimers.begin(); it != activeTimers.end();) {
        it->second -= Engine::Time::deltaTime;

        if (it->second <= 0.0f) {
            UpdatePlayerFlags(it->first, false);
            it = activeTimers.erase(it);
        }
        else {
            ++it;
        }
    }
}

void PlayerPowerUpController::AddPowerUp(PowerUpType type, float duration) {
    if (!player) return;
    activeTimers[type] = duration;
    UpdatePlayerFlags(type, true);
    std::cout << "[POWERUP CONTROLLER] Started/Refreshed Type " << (int)type << " for " << duration << "s\n";
}

void PlayerPowerUpController::UpdatePlayerFlags(PowerUpType type, bool isActive) {
    switch (type) {
    case PowerUpType::SpeedBoost:
        Player::hasSpeedBoost = isActive;
        break;
    case PowerUpType::InfiniteStamina:
        Player::hasInfiniteStamina = isActive;
        break;
    case PowerUpType::Shield:
        Player::hasShield = isActive;
        break;
    case PowerUpType::Invisibility:
        Player::isInvisible = isActive;
        break;
    }

    if (!isActive) {
        std::cout << "[POWERUP CONTROLLER] Type " << (int)type << " has worn off.\n";
    }
}

void PlayerPowerUpController::RemovePowerUp(PowerUpType type) {
    auto it = activeTimers.find(type);
    if (it != activeTimers.end()) {
        UpdatePlayerFlags(type, false);
        activeTimers.erase(it);
        std::cout << "[POWERUP CONTROLLER] Type " << (int)type << " was removed prematurely.\n";
    }
}

} // namespace Game