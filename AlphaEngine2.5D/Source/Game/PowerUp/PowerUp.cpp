#include "PowerUp.h"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Interaction/Interactable.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include <iostream>
#include <cmath>

namespace Game {

    void PowerUpObject::Initialize() {
        spriteRenderer = gameObject->GetComponent<Engine::SpriteRenderer>();
        interactable = gameObject->GetComponent<Engine::Interactable>();

        baseZ = gameObject->transform->position.z;

        if (spriteRenderer) {
            spriteRenderer->highlightColor = GetHighlightColor();
        }

        SetupInteraction();
    }

    void PowerUpObject::Update() {
        if (!isCollected && gameObject->transform) {
            floatTime += Engine::Time::deltaTime * floatSpeed;
            gameObject->transform->position.z = baseZ + std::sin(floatTime) * floatHeight;
            gameObject->transform->rotation += Engine::Time::deltaTime * 30.0f;
        }
    }

    void PowerUpObject::SetupInteraction() {
        if (!interactable) return;

        interactable->actionText = ("Collect " + reference.name);
        interactable->maxDistance = 2.5f;

        interactable->onInteract = [this]() {
            this->OnCollect();
            };
    }

    void PowerUpObject::OnCollect() {
        if (isCollected) return;

        auto playerObj = Engine::GameObject::Find("Player");
        if (playerObj) {
            // Send directly to the Controller to manage it natively!
            auto controller = playerObj->GetComponent<PlayerPowerUpController>();
            if (controller) {
                std::cout << "[POWERUP] Got " << reference.name << "!\n";
                controller->AddPowerUp(reference.type, reference.duration);

                isCollected = true;
                // Disable this world item to hide it and ignore further interactions
                gameObject->active = false;
            }
        }
    }

    Engine::Color PowerUpObject::GetHighlightColor() const {
        switch (reference.type) {
        case PowerUpType::SpeedBoost:      return Engine::Color(0.0f, 0.5f, 0.5f, 0.0f); // Cyan
        case PowerUpType::InfiniteStamina: return Engine::Color(0.8f, 0.4f, 0.0f, 0.0f); // Orange
        case PowerUpType::Shield:          return Engine::Color(0.0f, 0.0f, 0.8f, 0.0f); // Blue
        case PowerUpType::Invisibility:    return Engine::Color(0.8f, 0.8f, 0.8f, 0.0f); // Grey
        default: return Engine::Color(0.3f, 0.3f, 0.3f, 0.0f);
        }
    }

} // namespace Game