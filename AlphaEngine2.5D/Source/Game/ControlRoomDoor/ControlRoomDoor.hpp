/**
 * @file ControlRoomDoor.hpp
 * @brief Special door that requires access cards to unlock
 */
#pragma once

#include "Engine/Core/Component.hpp"

namespace Engine {
    class Door;
    class Interactable;
}

namespace Game {

    /**
     * @class ControlRoomDoor
     * @brief Door that requires all 4 access cards to unlock
     * 
     * This door leads to a control room with a CCTV terminal.
     * It remains locked until the player collects all 4 access cards.
     */
    class ControlRoomDoor : public Engine::Component {
    public:
        /** Zone ID this door belongs to (1-4) */
        int zoneId = 1;
        
        /** Required number of cards to unlock */
        int requiredCards = 4;
        
        /** References */
        Engine::Door* door = nullptr;
        Engine::Interactable* interactable = nullptr;
        
        /** Hold-to-unlock mechanic */
        float holdProgress = 0.0f;
        float holdTimeRequired = 2.0f;
        float holdDecayMultiplier = 2.0f;
        float interactMaxDistance = 2.5f;
        float lockedSfxVolume = 0.8f;
        float unlockSfxVolume = 0.8f;
        bool isHolding = false;
        
        // Lifecycle
        void Awake() override;
        void Update() override;
        
        /** Setup interaction */
        void SetupInteraction();
        
        /** Handle interaction (locked/unlock) */
        void OnInteract();
        
        /** Check if player has all required cards */
        bool HasAllCards() const;
        
        /** Get number of cards collected */
        int GetCollectedCards() const;
    };

} // namespace Game
