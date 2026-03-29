/*!
 * \file CCTVTerminal.hpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief Component that represents an interactive CCTV terminal in the world
 */

#pragma once
#include "Engine/Core/Component.hpp"
#include "Engine/Interaction/Interactable.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Graphics/Color.hpp"
#include <vector>
#include <string>

namespace Game {
    class CCTVCamera;
    class SecurityGuard;
    class PlayerHUD;
    
    /*!
     * \brief Convert zone ID (1-4) to zone letter (A-D)
     * \param zoneId [in] Zone numeric id
     * \return Letter representing zone
     */
    inline char ZoneIdToLetter(int zoneId) {
        switch (zoneId) {
            case 1: return 'A'; // Bottom-Left
            case 2: return 'B'; // Top-Left
            case 3: return 'C'; // Top-Right
            case 4: return 'D'; // Bottom-Right
            default: return '?';
        }
    }
    
    /*!
     * \brief Get zone name as string (e.g., "Zone A")
     */
    inline std::string GetZoneName(int zoneId) {
        return std::string("ZONE ") + ZoneIdToLetter(zoneId);
    }

    /*!
     * \enum TerminalState
     * \brief Visual and interaction state of a terminal
     */
    enum class TerminalState {
        ACTIVE,      // Blue/green glow - cameras online
        HACKING,     // Yellow pulsing - player interacting
        DISABLED,    // Red/dark - cameras offline
        REPAIRING    // Orange pulsing - guard fixing
    };

    /*!
     * \class CCTVTerminal
     * \brief Terminal component that can disable/restore camera networks and be repaired
     */
    class CCTVTerminal : public Engine::Component {
    public:
        // Configuration
        int controlledZone = 1; // 1-4
        
        // References
        std::vector<CCTVCamera*> controlledCameras;
        std::vector<SecurityGuard*> allGuards;
        PlayerHUD* playerHUD = nullptr;
        Engine::SpriteRenderer* spriteRenderer = nullptr;
        
        // State
        TerminalState state = TerminalState::ACTIVE;
        bool isDisabled = false;
        float disabledTimer = 0.0f;
        float disabledDuration = 30.0f;
        
        // Repair system
        bool guardIsRepairing = false;
        SecurityGuard* repairingGuard = nullptr;
        float repairProgress = 0.0f;
        float repairTime = 5.0f;
        
        // Interaction
        float interactionTime = 3.0f;
        float interactionProgress = 0.0f;
        bool playerIsInteracting = false;
        float cooldownTimer = 0.0f;
        float cooldownDuration = 10.0f;
        
        // Floating icon
        Engine::GameObject* floatingIcon = nullptr;
        float floatAnimTime = 0.0f;
        
        // Component lifecycle
        /*! \brief Initialize terminal (create interactable, icon). */
        void Awake() override;
        /*! \brief Per-frame update handling timers and visuals. */
        void Update() override;
        
        // Interaction handling
        /*! \brief Setup Interactable callback for hacking. */
        void SetupInteraction();
        /*! \brief Called when player interacts with terminal. */
        void OnTerminalInteract();
        /*! \brief Whether player can start hacking now. \return true if hackable */
        bool CanHack() const;
        
        // Terminal actions
        /*! \brief Disable all controlled cameras immediately. */
        void DisableCameras();
        /*! \brief Restore controlled cameras to active state. */
        void RestoreCameras();
        /*! \brief Start guard repair process. \param guard [in] Guard performing repair */
        void StartRepair(SecurityGuard* guard);
        /*! \brief Cancel active repair. */
        void CancelRepair();
        /*! \brief Progress repair by deltaTime. \param deltaTime [in] Time step */
        void UpdateRepair(float deltaTime);
        
        // Notification
        /*! \brief Notify nearby guards about a sabotage event. */
        void NotifyGuardsOfSabotage();
        
        // Visual
        /*! \brief Update sprite color/animation based on terminal state. */
        void UpdateVisualState();
        /*! \brief Get color representing current state. \return Color */
        Engine::Color GetStateColor() const;
        
    private:
        void UpdateDisabledState(float deltaTime);
        void UpdateCooldown(float deltaTime);
    };

} // namespace Game
