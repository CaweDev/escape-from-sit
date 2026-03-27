#include "PlayerPowerUpController.hpp"
#include "Game/Player/Player.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <iostream>

namespace
{
    struct PlayerPowerUpControllerConfig : public Engine::IConfig
    {
        float expireEpsilon = 0.0f;
        bool logEnabled = true;

        void FromJson(const Json::Value& j) override
        {
            expireEpsilon = j.get("expireEpsilon", expireEpsilon).asFloat();
            logEnabled = j.get("logEnabled", logEnabled).asBool();
        }
    };

    PlayerPowerUpControllerConfig g_playerPowerUpConfig;
    bool g_playerPowerUpConfigLoaded = false;

    const PlayerPowerUpControllerConfig& GetPlayerPowerUpControllerConfig()
    {
        if (!g_playerPowerUpConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/player_powerup_controller.json", g_playerPowerUpConfig);
            g_playerPowerUpConfigLoaded = true;
        }
        return g_playerPowerUpConfig;
    }
}

namespace Game {

void PlayerPowerUpController::Awake() {
    player = gameObject->GetComponent<Player>();
}

void PlayerPowerUpController::Update() {
    if (!player) return;
    const auto& cfg = GetPlayerPowerUpControllerConfig();

    for (auto it = activeTimers.begin(); it != activeTimers.end();) {
        it->second -= Engine::Time::deltaTime;

        if (it->second <= cfg.expireEpsilon) {
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
    if (GetPlayerPowerUpControllerConfig().logEnabled)
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
        if (GetPlayerPowerUpControllerConfig().logEnabled)
            std::cout << "[POWERUP CONTROLLER] Type " << (int)type << " has worn off.\n";
    }
}

void PlayerPowerUpController::RemovePowerUp(PowerUpType type) {
    auto it = activeTimers.find(type);
    if (it != activeTimers.end()) {
        UpdatePlayerFlags(type, false);
        activeTimers.erase(it);
        if (GetPlayerPowerUpControllerConfig().logEnabled)
            std::cout << "[POWERUP CONTROLLER] Type " << (int)type << " was removed prematurely.\n";
    }
}

} // namespace Game