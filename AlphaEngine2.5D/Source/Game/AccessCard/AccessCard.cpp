/**
 * @file AccessCard.cpp
 * @brief Implementation of AccessCard collectible
 */
#include "AccessCard.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Interaction/Interactable.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include "Game/Player/Player.hpp"
#include "Game/Scenes/FPSScene/UI/PlayerHUD.hpp"
#include "Engine/Audio/SoundManager.hpp"
#include <iostream>
#include <cmath>

namespace Game {

void AccessCard::Awake() {
    // Cache component references
    spriteRenderer = gameObject->GetComponent<Engine::SpriteRenderer>();
    interactable = gameObject->GetComponent<Engine::Interactable>();
    
    // Store base Z position
    baseZ = gameObject->transform->position.z;
    
    // Set highlight color based on card type
    if (spriteRenderer) {
        spriteRenderer->highlightColor = GetHighlightColor();
    }
    
    // Setup interaction
    SetupInteraction();
}

void AccessCard::Update() {
    // Floating animation
    floatTime += Engine::Time::deltaTime * floatSpeed;
    gameObject->transform->position.z = baseZ + std::sin(floatTime) * floatHeight;
    
    // Gentle rotation
    gameObject->transform->rotation += Engine::Time::deltaTime * 30.0f;

    // Update spawn safe timer
    if (spawnSafeTimer < spawnSafeDelay) spawnSafeTimer += Engine::Time::deltaTime;
}

void AccessCard::SetupInteraction() {
    if (!interactable) return;

    // Set interaction callback with spawn-safety guard
    interactable->onInteract = [this]() {
        // ignore interactions during short spawn-safe period to avoid instant pickup
        if (spawnSafeTimer < spawnSafeDelay) {
            std::cout << "[ACCESS CARD] Interaction ignored: spawn safe timer (" << spawnSafeTimer << ")\n";
            return;
        }
        this->OnCollect();
    };
}

void AccessCard::OnCollect() {
    std::cout << "[ACCESS CARD] Collected " << GetColorName() << " access card!\n";
    
    // Find player and notify HUD
    auto player = Engine::GameObject::Find("Player");
    if (player) {
        auto hud = player->GetComponent<PlayerHUD>();
        if (hud) {
            Engine::Vector2 cardPos(gameObject->transform->position.x, gameObject->transform->position.y);
            hud->CollectAccessCard(cardId, cardPos);
        }
    }
    
    // Hide the card instead of destroying it (like books do)
    // This allows it to respawn when the scene reloads
    gameObject->active = false;
    // Play collect SFX
    Engine::SoundManager::PlaySFX("card_collect");
}

const char* AccessCard::GetColorName() const {
    switch (cardColor) {
        case AccessCardColor::Green:  return "Green";
        case AccessCardColor::Purple: return "Purple";
        case AccessCardColor::Red:    return "Red";
        case AccessCardColor::Yellow: return "Yellow";
        default: return "Unknown";
    }
}

Engine::Color AccessCard::GetHighlightColor() const {
    switch (cardColor) {
        case AccessCardColor::Green:  return Engine::Color(0.0f, 0.5f, 0.0f, 0.0f);
        case AccessCardColor::Purple: return Engine::Color(0.5f, 0.0f, 0.5f, 0.0f);
        case AccessCardColor::Red:    return Engine::Color(0.5f, 0.0f, 0.0f, 0.0f);
        case AccessCardColor::Yellow: return Engine::Color(0.5f, 0.5f, 0.0f, 0.0f);
        default: return Engine::Color(0.3f, 0.3f, 0.3f, 0.0f);
    }
}

} // namespace Game
