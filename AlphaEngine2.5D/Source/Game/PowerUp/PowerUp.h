#pragma once
#include "Engine/Core/Component.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Graphics/Color.hpp"
#include "Game/Player/PlayerPowerUpController.h"
#include <string>

namespace Engine {
    class SpriteRenderer;
    class Interactable;
}

namespace Game {

    struct PowerUpData {
        int id;
        Engine::Vector3 position;
        PowerUpType type;
        std::string name;
        float duration;
    };

    /**
     * Floating physical collectible that sits in the map
     */
    class PowerUpObject : public Engine::Component {
    public:
        PowerUpData reference;
        bool isCollected = false;

        float floatTime = 0.0f;
        float baseZ = 0.0f;
        float floatHeight = 0.15f;
        float floatSpeed = 2.0f;

        // Respawn settings
        bool allowRespawn = true;
        float respawnDelay = 20.0f;
        float respawnTimer = 0.0f;

        Engine::SpriteRenderer* spriteRenderer = nullptr;
        Engine::Interactable* interactable = nullptr;

        void Initialize() override;
        void Update() override;
        void SetupInteraction();
        void OnCollect();

        Engine::Color GetHighlightColor() const;

    private:
        void SetPickupEnabled(bool enabled);
        void Respawn();
    };

} // namespace Game