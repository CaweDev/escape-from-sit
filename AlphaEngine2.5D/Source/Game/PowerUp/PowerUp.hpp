/**
 * @file PowerUp.hpp
 * @brief Header for the PowerUpObject component and PowerUpData structure.
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

    /**
     * @struct PowerUpData
     * @brief Data structure for defining power-up properties.
     */
    struct PowerUpData {
        int id;                     ///< Unique ID for the power-up.
        Engine::Vector3 position;   ///< World position of the power-up.
        PowerUpType type;           ///< Type of power-up (from PlayerPowerUpController).
        std::string name;           ///< Display name of the power-up.
        float duration;             ///< Duration of the effect in seconds.
    };

    /**
     * @class PowerUpObject
     * @brief Floating physical collectible that sits in the map.
     */
    class PowerUpObject : public Engine::Component {
    public:
        PowerUpData reference;      ///< Base data for this power-up instance.
        bool isCollected = false;   ///< True if the power-up is currently collected.

        float floatTime = 0.0f;     ///< Time elapsed for floating animation.
        float baseZ = 0.0f;         ///< Base Z-height of the object.
        float floatHeight = 0.15f;  ///< Amplitude of floating motion.
        float floatSpeed = 2.0f;    ///< Frequency of floating motion.

        // Respawn settings
        bool allowRespawn = true;   ///< Whether the power-up can respawn after collection.
        float respawnDelay = 20.0f; ///< Delay in seconds before respawning.
        float respawnTimer = 0.0f;  ///< Current time remaining until respawn.

        Engine::SpriteRenderer* spriteRenderer = nullptr; ///< Reference to the sprite renderer.
        Engine::Interactable* interactable = nullptr;     ///< Reference to the interactable component.

        /**
         * @brief Initializes the power-up object.
         */
        void Initialize() override;

        /**
         * @brief Updates the floating animation and respawn timer.
         */
        void Update() override;

        /**
         * @brief Sets up the interactable component for the power-up.
         */
        void SetupInteraction();

        /**
         * @brief Called when the player collects the power-up.
         */
        void OnCollect();

        /**
         * @brief Gets the highlight color for the power-up based on its type.
         * @return Engine::Color The highlight color.
         */
        Engine::Color GetHighlightColor() const;

    private:
        /**
         * @brief Enables or disables the pickup interaction and visibility.
         * @param enabled True to enable, false to disable.
         */
        void SetPickupEnabled(bool enabled);

        /**
         * @brief Respawns the power-up at its original position.
         */
        void Respawn();
    };

} // namespace Game
