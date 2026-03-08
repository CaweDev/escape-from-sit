#include "Player.hpp"
#include "Game/Scenes/FPSScene/UI/Hands67.hpp"
namespace Game {
	void Player::Catch()
	{
		bool immune = false;
		{
			Hands67* hands = gameObject->GetComponent<Hands67>();
			if (hands->IsVisible()) {
				immune = true;
			}
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
}