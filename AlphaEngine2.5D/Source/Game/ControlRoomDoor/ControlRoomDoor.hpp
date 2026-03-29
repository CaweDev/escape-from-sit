/*!
 * \file ControlRoomDoor.hpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief Special door that requires access cards to unlock
 */
#pragma once

#include "Engine/Core/Component.hpp"

namespace Engine {
    class Door;
    class Interactable;
}

namespace Game {

    /*!
     * \class ControlRoomDoor
     * \brief Door that requires a set of access cards to unlock the control room
     *
     * The door exposes an interactable which the player holds to attempt unlock.
     */
    class ControlRoomDoor : public Engine::Component {
    public:
        /*! \brief Zone ID this door belongs to (1-4) */
        int zoneId = 1;
        
        /*! \brief Required number of cards to unlock */
        int requiredCards = 4;
        
        /*! \brief References to door component and interactable */
        Engine::Door* door = nullptr;
        Engine::Interactable* interactable = nullptr;
        
        // Hold-to-unlock mechanic
        float holdProgress = 0.0f;
        float holdTimeRequired = 2.0f;
        float holdDecayMultiplier = 2.0f;
        float interactMaxDistance = 2.5f;
        float lockedSfxVolume = 0.8f;
        float unlockSfxVolume = 0.8f;
        bool isHolding = false;
        
        // Lifecycle
        /*! \brief Called when component is created to set up interactable. */
        void Awake() override;
        /*! \brief Per-frame update to handle hold/unlock progression. */
        void Update() override;
        
        /*! \brief Configure interaction callback. */
        void SetupInteraction();
        
        /*! \brief Handle player interaction attempt. */
        void OnInteract();
        
        /*! \brief Query whether player has collected all required cards. \return true if collected */
        bool HasAllCards() const;
        
        /*! \brief Return number of collected cards. */
        int GetCollectedCards() const;
    };

} // namespace Game
