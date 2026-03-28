#include "Player.hpp"
#include "Game/Scenes/FPSScene/UI/Hands67.hpp"
#include "Game/Player/PlayerPowerUpController.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <iostream>

namespace
{
	struct PlayerConfig : public Engine::IConfig
	{
		bool hands67ProvidesImmunity = true;
		bool logShieldAbsorb = true;
		bool logInvulnerabilityEnd = true;

		void FromJson(const Json::Value& j) override
		{
			hands67ProvidesImmunity = j.get("hands67ProvidesImmunity", hands67ProvidesImmunity).asBool();
			logShieldAbsorb = j.get("logShieldAbsorb", logShieldAbsorb).asBool();
			logInvulnerabilityEnd = j.get("logInvulnerabilityEnd", logInvulnerabilityEnd).asBool();
		}
	};

	PlayerConfig g_playerConfig;
	bool g_playerConfigLoaded = false;

	const PlayerConfig& GetPlayerConfig()
	{
		if (!g_playerConfigLoaded)
		{
			Engine::ConfigManager::ReadConfig("Assets/Config/Game/player.json", g_playerConfig);
			g_playerConfigLoaded = true;
		}
		return g_playerConfig;
	}
}

namespace Game {

	bool Player::cheatInfiniteSpeed = false;
	bool Player::cheatCatchImmune   = false;

	bool Player::hasSpeedBoost = false;
	bool Player::hasInfiniteStamina = false;
	bool Player::hasShield = false;
	bool Player::isInvisible = false;

	void Player::Catch()
	{
		bool immune = false;
		{
			Hands67* hands = gameObject->GetComponent<Hands67>();
			if (GetPlayerConfig().hands67ProvidesImmunity && hands->IsVisible()) {
				immune = true;
			}
		}

		if(Player::hasShield) {
			auto* controller = gameObject->GetComponent<PlayerPowerUpController>();

			if (controller) {
				controller->RemovePowerUp(PowerUpType::Shield);
			}
			else {
				// Fallback: keep state consistent even if controller is missing
				Player::hasShield = false;
			}

			invulnerabilityTimer = shieldBreakIFrameDuration; // Start invulnerability frames
			immune = true;
			/*if (GetPlayerConfig().logShieldAbsorb)
				std::cout << "[PLAYER] Shield absorbed the catch! Player is now invulnerable for " << shieldBreakIFrameDuration << " seconds.\n";*/
		}
		if (cheatCatchImmune) {
			immune = true;
		}
		if (!immune) {
			state = PlayerState::Caught;
		}
	}

	void Player::Win()
	{
		state = PlayerState::Won;
	}

	bool Player::IsCaught() const
	{
		return state == PlayerState::Caught;
	}

	bool Player::IsWon() const
	{
		return state == PlayerState::Won;
	}

	PlayerState Player::GetState() const
	{
		return state;
	}

	bool Player::CanSprint() const
	{
		return stamina >= sprintResumeThreshold;
	}

	float Player::GetStamina() const
	{
		return stamina;
	}

	float Player::GetStaminaMax() const
	{
		return staminaMax;
	}

	void Player::DrainSprint(float dt)
	{
		stamina -= staminaDrainPerSec * dt;
		if (stamina < 0.0f) {
			stamina = 0.0f;
		}
	}

	void Player::Regen(float dt)
	{
		stamina += staminaRegenPerSec * dt;
		if (stamina > staminaMax) {
			stamina = staminaMax;
		}
	}

	void Player::ResetStamina()
	{
		stamina = staminaMax;
	}
	void Player::AddKnockback(const Engine::Vector2& v)
	{
		knockback.x += v.x;
		knockback.y += v.y;
	}

	Engine::Vector2 Player::ConsumeKnockback()
	{
		Engine::Vector2 out = knockback;
		knockback = Engine::Vector2(0.0f, 0.0f);
		return out;
	}

	void Player::UpdateGameTime(float dt) {
		gameTime += dt;

		// Tick down invulnerability frames
		if (invulnerabilityTimer > 0.0f) {
			invulnerabilityTimer -= dt;
			if (invulnerabilityTimer <= 0.0f) {
				/*if (GetPlayerConfig().logInvulnerabilityEnd)
					std::cout << "[PLAYER] Invulnerability wore off!\n";*/
			}
		}
	}
}