/**
 * @file ControlRoomDoor.cpp
 * @brief Implementation of ControlRoomDoor component
 */
#include "ControlRoomDoor.hpp"
#include "Engine/TileMap/Door.hpp"
#include "Engine/Interaction/Interactable.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Game/Player/Player.hpp"
#include "Game/Scenes/FPSScene/UI/PlayerHUD.hpp"
#include <iostream>

namespace Game {

void ControlRoomDoor::Awake() {
    // Cache references
    door = gameObject->GetComponent<Engine::Door>();
    interactable = gameObject->GetComponent<Engine::Interactable>();
    
    // Ensure door starts closed and locked
    if (door) {
        door->state = Engine::DoorState::Closed;
        door->openAmount = 0.0f;
    }
    
    // Setup interaction
    SetupInteraction();
}

void ControlRoomDoor::Update() {
    // Update interaction text based on card collection status
    if (!interactable) return;
    
    if (HasAllCards()) {
        // Player has all cards - door can be opened normally
        if (door) {
            if (door->state == Engine::DoorState::Closed || door->state == Engine::DoorState::Closing) {
                interactable->actionText = "Open Control Room";
            } else {
                interactable->actionText = "Close";
            }
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
    
    // Player has all cards - allow door to function normally
    door->Toggle();
    
    if (door->state == Engine::DoorState::Opening || door->state == Engine::DoorState::Open) {
        std::cout << "[CONTROL ROOM] Door unlocked! Welcome to Zone " << char('A' + zoneId - 1) << " Control Room.\n";
    }
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
