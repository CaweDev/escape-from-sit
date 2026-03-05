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
    
    /**
     * @brief Convert zone ID (1-4) to zone letter (A-D)
     * Zone layout:
     *   Top-Left: 2 ? B
     *   Top-Right: 3 ? C
     *   Bottom-Left: 1 ? A
     *   Bottom-Right: 4 ? D
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
    
    /**
     * @brief Get zone name as string (e.g., "Zone A")
     */
    inline std::string GetZoneName(int zoneId) {
        return std::string("ZONE ") + ZoneIdToLetter(zoneId);
    }

    enum class TerminalState {
        ACTIVE,      // Blue/green glow - cameras online
        HACKING,     // Yellow pulsing - player interacting
        DISABLED,    // Red/dark - cameras offline
        REPAIRING    // Orange pulsing - guard fixing
    };

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
        void Awake() override;
        void Update() override;
        
        // Interaction handling
        void SetupInteraction();
        void OnTerminalInteract();
        bool CanHack() const;
        
        // Terminal actions
        void DisableCameras();
        void RestoreCameras();
        void StartRepair(SecurityGuard* guard);
        void CancelRepair();
        void UpdateRepair(float deltaTime);
        
        // Notification
        void NotifyGuardsOfSabotage();
        
        // Visual
        void UpdateVisualState();
        Engine::Color GetStateColor() const;
        
    private:
        void UpdateDisabledState(float deltaTime);
        void UpdateCooldown(float deltaTime);
    };

} // namespace Game
