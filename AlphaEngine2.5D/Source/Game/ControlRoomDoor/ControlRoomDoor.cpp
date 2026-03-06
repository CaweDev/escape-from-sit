/**
 * @file ControlRoomDoor.cpp
 * @brief Implementation of ControlRoomDoor component
 */
#include "ControlRoomDoor.hpp"
#include "Engine/TileMap/Door.hpp"
#include "Engine/Interaction/Interactable.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Core/Time.hpp"
#include "Game/Player/Player.hpp"
#include "Game/Scenes/FPSScene/UI/PlayerHUD.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace Game {

void ControlRoomDoor::Awake() {
    // Cache references
    door = gameObject->GetComponent<Engine::Door>();
    interactable = gameObject->GetComponent<Engine::Interactable>();
    
    // Ensure door starts closed and locked
    if (door) {
        door->state = Engine::DoorState::Closed;
        door->openAmount = 0.0f;
        
        // CRITICAL: Disable Door's own interaction callback!
        // ControlRoomDoor will handle ALL interactions
        door->DisableInteraction();
    }
    
    // Setup interaction (ControlRoomDoor takes full control)
    SetupInteraction();
}

void ControlRoomDoor::Update() {
    // Update interaction text based on card collection status
    if (!interactable) return;
    
    // Handle hold-to-unlock mechanic
    if (HasAllCards() && door && door->state == Engine::DoorState::Closed) {
        // Check if player is nearby by finding player and checking distance
        auto player = Engine::GameObject::Find("Player");
        bool playerNearby = false;
        
        if (player && player->transform) {
            float dx = player->transform->position.x - transform->position.x;
            float dy = player->transform->position.y - transform->position.y;
            float distance = std::sqrt(dx * dx + dy * dy);
            playerNearby = (distance <= interactable->maxDistance);
        }
        
        bool ePressed = Engine::Input::GetKey(Engine::KeyCode::E);
        
        if (playerNearby && ePressed) {
            isHolding = true;
            holdProgress += Engine::Time::deltaTime / holdTimeRequired;
            
            if (holdProgress >= 1.0f) {
                // Unlock achieved!
                door->Toggle();
                std::cout << "[CONTROL ROOM] Door unlocked! Welcome to Zone " << char('A' + zoneId - 1) << " Control Room.\n";
                holdProgress = 0.0f;
                isHolding = false;
            }
        } else {
            // Reset progress if not holding
            isHolding = false;
            holdProgress = std::max(0.0f, holdProgress - Engine::Time::deltaTime * 2.0f); // Decay faster
        }
    }
    
    // Update interaction text
    if (HasAllCards()) {
        if (door && door->state != Engine::DoorState::Closed) {
            interactable->actionText = "Close";
        } else if (isHolding && holdProgress > 0.0f) {
            int percent = (int)(holdProgress * 100.0f);
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "Unlocking... %d%%", percent);
            interactable->actionText = buffer;
        } else {
            interactable->actionText = "Hold E to Unlock Control Room";
        }
    } else {
        // Door is locked - show progress
        int collected = GetCollectedCards();
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "LOCKED - Access Cards Required (%d/%d)", collected, REQUIRED_CARDS);
        interactable->actionText = buffer;
    }
}

void ControlRoomDoor::SetupInteraction() {
    if (!interactable) return;
    
    interactable->actionText = "LOCKED - Access Cards Required (0/4)";
    interactable->maxDistance = 2.5f;
    
    // Override interaction callback
    interactable->onInteract = [this]() {
        this->OnInteract();
    };
}

void ControlRoomDoor::OnInteract() {
    if (!door) return;
    
    if (!HasAllCards()) {
        // Show locked message
        int collected = GetCollectedCards();
        std::cout << "[CONTROL ROOM] Door is LOCKED. Access cards collected: " << collected << "/" << REQUIRED_CARDS << "\n";
        std::cout << "[CONTROL ROOM] Find all " << REQUIRED_CARDS << " access cards to unlock this door.\n";
        return;
    }
    
    // Allow toggle if door is already open (for closing)
    if (door->state != Engine::DoorState::Closed) {
        door->Toggle();
    }
    // Door opening is handled by hold mechanic in Update()
}

bool ControlRoomDoor::HasAllCards() const {
    return GetCollectedCards() >= REQUIRED_CARDS;
}

int ControlRoomDoor::GetCollectedCards() const {
    // Find player and check HUD for card collection status
    auto player = Engine::GameObject::Find("Player");
    if (!player) return 0;
    
    auto hud = player->GetComponent<PlayerHUD>();
    if (!hud) return 0;
    
    return hud->GetCollectedCardCount();
}

} // namespace Game
