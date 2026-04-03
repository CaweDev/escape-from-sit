/*!
 * \file PowerUp.hpp
 * \author Chim Kin Ho, Caleb
 * \date 2026-03-29
 * \brief Header for power-up collectible component and its data
 */
#pragma once

#include "Engine/Core/Component.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Graphics/Color.hpp"
#include "Game/Player/PlayerPowerUpController.hpp"
#include <string>

namespace Engine {
    class SpriteRenderer;
    class Interactable;
}

namespace Game {

    /*!
     * \struct PowerUpData
     * \brief Data that defines a power-up instance
     */
    struct PowerUpData {
        int id = 0;                     /*!< Unique ID for the power-up. */
        Engine::Vector3 position;   /*!< World position of the power-up. */
        PowerUpType type = PowerUpType::SpeedBoost;           /*!< Type of power-up (from PlayerPowerUpController). */
        std::string name;           /*!< Display name of the power-up. */
        float duration = 0;             /*!< Duration of the effect in seconds. */
    };

    /*!
     * \class PowerUpObject
     * \brief Floating collectible that grants a temporary player effect
     */
    class PowerUpObject : public Engine::Component {
    public:
        PowerUpData reference;      /*!< Base data for this power-up instance. */
        bool isCollected = false;   /*!< True if the power-up is currently collected. */

        float floatTime = 0.0f;     /*!< Time elapsed for floating animation. */
        float baseZ = 0.0f;         /*!< Base Z-height of the object. */
        float floatHeight = 0.15f;  /*!< Amplitude of floating motion. */
        float floatSpeed = 2.0f;    /*!< Frequency of floating motion. */

        // Respawn settings
        bool allowRespawn = true;   /*!< Whether the power-up can respawn after collection. */
        float respawnDelay = 20.0f; /*!< Delay in seconds before respawning. */
        float respawnTimer = 0.0f;  /*!< Current time remaining until respawn. */

        Engine::SpriteRenderer* spriteRenderer = nullptr; /*!< Sprite renderer reference. */
        Engine::Interactable* interactable = nullptr;     /*!< Interactable reference. */

        /*! \brief Initialize the power-up (components, visuals). */
        void Initialize() override;

        /*! \brief Update floating animation and respawn timer. */
        void Update() override;

        /*! \brief Setup Interactable callback for player pickup. */
        void SetupInteraction();

        /*! \brief Called when the player collects the power-up. */
        void OnCollect();

        /*! \brief Get highlight color representing power-up type. \return Color */
        Engine::Color GetHighlightColor() const;

    private:
        /*! \brief Enable or disable pickup visibility and interaction. \param enabled [in] */
        void SetPickupEnabled(bool enabled);

        /*! \brief Respawn the power-up at its reference position. */
        void Respawn();
    };

} // namespace Game
