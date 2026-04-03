/*!
 * \file AlphaEngine2.5D\Source\Game\Player\Player.hpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief [AUTO] Added Doxygen file header
 */

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
		void UpdateGameTime(float dt);
		void ResetGameTime() { gameTime = 0.0f; }
		
		float GetHighScore() const { return highScore; }
		void SetHighScore(float score) { highScore = score; }


		// Invulnerability check
		bool IsInvulnerable() const { return invulnerabilityTimer > 0.0f; }

		bool isSprinting = false;

		// Power-up flags
		static bool hasSpeedBoost;
		static bool hasInfiniteStamina;
		static bool hasShield;
		static bool isInvisible;

		// Accessor for invisibility state (use this instead of direct flag checks)
		static bool IsInvisibleActive();

		// Cheat flags (toggled by DebugMenu, only active in FPSScene)
		static bool cheatInfiniteSpeed;
		static bool cheatCatchImmune;

	private:
		PlayerState state = PlayerState::Normal;
		Engine::Vector2 knockback{ 0.0f, 0.0f };
		float stamina = 100.0f;
		float staminaMax = 100.0f;
		float sprintResumeThreshold = 5.0f;  // Lowered from 20.0f - player can sprint again at 5% stamina
		float staminaDrainPerSec = 10.0f;
		float staminaRegenPerSec = 5.0f;
		
		float gameTime = 0.0f;
		float highScore = 999999.0f; // Large number means no record yet

		// shield break invuln
		float invulnerabilityTimer = 0.0f;
		const float shieldBreakIFrameDuration = 2.0f; // 2 seconds of safety after shield breaks
	};

}