#pragma once

#include "Engine.hpp"
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

		// Knockback (set by NPC/Bully)
		void AddKnockback(const Engine::Vector2& v);
		Engine::Vector2 ConsumeKnockback();
		
		// Game timer and high score
		float GetGameTime() const { return gameTime; }
		void UpdateGameTime(float dt) { gameTime += dt; }
		void ResetGameTime() { gameTime = 0.0f; }
		
		float GetHighScore() const { return highScore; }
		void SetHighScore(float score) { highScore = score; }

		bool isSprinting = false;

		// Cheat flags (toggled by DebugMenu, only active in FPSScene)
		static bool cheatInfiniteSpeed;
		static bool cheatCatchImmune;

	private:
		PlayerState state = PlayerState::Normal;
		Engine::Vector2 knockback{ 0.0f, 0.0f };
		float stamina = 100.0f;
		float staminaMax = 100.0f;
		float sprintResumeThreshold = 20.0f;
		float staminaDrainPerSec = 10.0f;
		float staminaRegenPerSec = 5.0f;
		
		float gameTime = 0.0f;
		float highScore = 999999.0f; // Large number means no record yet
	};

}