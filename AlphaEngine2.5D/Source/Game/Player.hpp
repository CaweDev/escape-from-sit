#pragma once

#include "Engine/Core/Component.hpp"

namespace Game {

	enum class PlayerState {
		Normal=0,
		Caught,
		Won
	};

	class Player : public Engine::Component {
	public:
		void Catch();
		void Win();
		bool IsCaught() const;
		bool IsWon() const;
		PlayerState GetState() const;

		bool CanSprint() const;
		float GetStamina() const;
		float GetStaminaMax() const;

		void DrainSprint(float dt);
		void Regen(float dt);
		void ResetStamina();

		bool isSprinting = false;

	private:
		PlayerState state = PlayerState::Normal;
		
		float stamina = 100.0f;
		float staminaMax = 100.0f;
		float sprintResumeThreshold = 20.0f;
		float staminaDrainPerSec = 10.0f;
		float staminaRegenPerSec = 5.0f;
	};

}