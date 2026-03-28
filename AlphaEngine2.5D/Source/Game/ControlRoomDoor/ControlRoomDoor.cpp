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
#include "Engine/Audio/SoundManager.hpp"
#include "Game/Player/Player.hpp"
#include "Game/Scenes/FPSScene/UI/PlayerHUD.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace
{
    struct ControlRoomDoorConfig : public Engine::IConfig
    {
        int requiredCards = 4;
        float holdTimeRequired = 2.0f;
        float holdDecayMultiplier = 2.0f;
        float interactMaxDistance = 2.5f;
        float lockedSfxVolume = 0.8f;
        float unlockSfxVolume = 0.8f;

        void FromJson(const Json::Value& j) override
        {
            requiredCards = j.get("requiredCards", requiredCards).asInt();
            holdTimeRequired = j.get("holdTimeRequired", holdTimeRequired).asFloat();
            holdDecayMultiplier = j.get("holdDecayMultiplier", holdDecayMultiplier).asFloat();
            interactMaxDistance = j.get("interactMaxDistance", interactMaxDistance).asFloat();
            lockedSfxVolume = j.get("lockedSfxVolume", lockedSfxVolume).asFloat();
            unlockSfxVolume = j.get("unlockSfxVolume", unlockSfxVolume).asFloat();
        }
    };

    ControlRoomDoorConfig g_controlRoomDoorConfig;
    bool g_controlRoomDoorConfigLoaded = false;

    const ControlRoomDoorConfig& GetControlRoomDoorConfig()
    {
        if (!g_controlRoomDoorConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/control_room_door.json", g_controlRoomDoorConfig);
            if (g_controlRoomDoorConfig.requiredCards < 1) g_controlRoomDoorConfig.requiredCards = 1;
            if (g_controlRoomDoorConfig.holdTimeRequired < 0.01f) g_controlRoomDoorConfig.holdTimeRequired = 0.01f;
            if (g_controlRoomDoorConfig.holdDecayMultiplier < 0.0f) g_controlRoomDoorConfig.holdDecayMultiplier = 0.0f;
            if (g_controlRoomDoorConfig.interactMaxDistance < 0.1f) g_controlRoomDoorConfig.interactMaxDistance = 0.1f;
            g_controlRoomDoorConfigLoaded = true;
        }
        return g_controlRoomDoorConfig;
    }
}

namespace Game {

void ControlRoomDoor::Awake() {
    const auto& cfg = GetControlRoomDoorConfig();
    requiredCards = cfg.requiredCards;
    holdTimeRequired = cfg.holdTimeRequired;
    holdDecayMultiplier = cfg.holdDecayMultiplier;
    interactMaxDistance = cfg.interactMaxDistance;
    lockedSfxVolume = cfg.lockedSfxVolume;
    unlockSfxVolume = cfg.unlockSfxVolume;

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
                Engine::SoundManager::PlaySFX("door_unlock", unlockSfxVolume);
                //std::cout << "[CONTROL ROOM] Door unlocked! Welcome to Zone " << char('A' + zoneId - 1) << " Control Room.\n";
                holdProgress = 0.0f;
                isHolding = false;
            }
        } else {
            // Reset progress if not holding
            isHolding = false;
            holdProgress = Engine::Mathf::Max(0.0f, holdProgress - Engine::Time::deltaTime * holdDecayMultiplier);
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
        snprintf(buffer, sizeof(buffer), "LOCKED - Access Cards Required (%d/%d)", collected, requiredCards);
        interactable->actionText = buffer;
    }
}

void ControlRoomDoor::SetupInteraction() {
    if (!interactable) return;
    
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "LOCKED - Access Cards Required (0/%d)", requiredCards);
        interactable->actionText = buffer;
    }
    interactable->maxDistance = interactMaxDistance;
    
    // Override interaction callback
    interactable->onInteract = [this]() {
        this->OnInteract();
    };
}

void ControlRoomDoor::OnInteract() {
    if (!door) return;
    
    if (!HasAllCards()) {
        // Show locked message
        Engine::SoundManager::PlaySFX("door_locked", lockedSfxVolume);
        //std::cout << "[CONTROL ROOM] Door is LOCKED. Access cards collected: " << collected << "/" << requiredCards << "\n";
        //std::cout << "[CONTROL ROOM] Find all " << requiredCards << " access cards to unlock this door.\n";
        return;
    }
    
    // Allow toggle if door is already open (for closing)
    if (door->state == Engine::DoorState::Open) {
        door->Toggle();
        Engine::SoundManager::PlaySFX("door_unlock", unlockSfxVolume);
    }
    // Door opening is handled by hold mechanic in Update()
}

bool ControlRoomDoor::HasAllCards() const {
    return GetCollectedCards() >= requiredCards;
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
