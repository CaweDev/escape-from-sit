#include "CCTVTerminal.hpp"
#include "CCTVCamera.hpp"
#include "Game/SecurityGuard/SecurityGuard.hpp"
#include "Game/Scenes/FPSScene/UI/PlayerHUD.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Interaction/Interactable.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/Audio/SoundManager.hpp"
#include <iostream>
#include <cmath>

namespace Game {

void CCTVTerminal::Awake() {
    std::cout << "[CCTVTerminal::Awake] Terminal spawned for Zone " << controlledZone << "\n";
    

    UpdateVisualState();
    SetupInteraction();
}

void CCTVTerminal::SetupInteraction() {
    std::cout << "[CCTVTerminal::SetupInteraction] Setting up interaction for Zone " << controlledZone << "\n";
    
    // Add Interactable component
    auto interactable = gameObject->GetComponent<Engine::Interactable>();
    if (!interactable) {
        interactable = gameObject->AddComponent<Engine::Interactable>();
        std::cout << "[CCTVTerminal::SetupInteraction] Created new Interactable component\n";
    } else {
        std::cout << "[CCTVTerminal::SetupInteraction] Using existing Interactable component\n";
    }
    
    interactable->actionText = "Hold E to Hack Terminal";
    interactable->maxDistance = 2.5f;
    interactable->requireHold = true;
    interactable->holdDuration = 3.0f;
    interactable->onInteract = [this]() { OnTerminalInteract(); };
    
    std::cout << "[CCTVTerminal::SetupInteraction] Interaction setup complete!\n";
}

void CCTVTerminal::OnTerminalInteract() {
    std::cout << "[CCTVTerminal] OnTerminalInteract called! Zone=" << controlledZone 
              << " (" << ZoneIdToLetter(controlledZone) << ")"
              << " isDisabled=" << isDisabled << " cooldown=" << cooldownTimer 
              << " playerHUD=" << (playerHUD ? "YES" : "NULL") << "\n";
              
    if (!CanHack()) {
        std::cout << "[CCTVTerminal] CanHack returned false!\n";
        return;
    }
    
    Engine::SoundManager::PlaySFX("terminal_hack");

    // For now, instant hack (TODO: add hold-E mechanic)
    DisableCameras();
    // Play cctv disabled SFX
    Engine::SoundManager::PlaySFX("cctv_disabled");
    
    // Provide HUD feedback
    if (playerHUD) {
        char zoneLetter = ZoneIdToLetter(controlledZone);
        std::string message = std::string("ZONE ") + zoneLetter + " CAMERAS DISABLED FOR 30s";
        std::cout << "[CCTVTerminal] Showing HUD message: " << message << "\n";
        playerHUD->ShowHackStatus(message, 3.0f);
    } else {
        std::cout << "[CCTVTerminal] playerHUD is NULL! Message not shown!\n";
    }
}

bool CCTVTerminal::CanHack() const {
    if (isDisabled) return false;
    if (cooldownTimer > 0.0f) return false;
    if (guardIsRepairing) return false;
    return true;
}

void CCTVTerminal::Update() {
// Update floating icon animation
if (floatingIcon) {
    floatAnimTime += Engine::Time::deltaTime;
    float floatOffset = std::sin(floatAnimTime * 2.0f) * 0.15f;
    floatingIcon->transform->position = Engine::Vector3(
        transform->position.x,
        transform->position.y,
        transform->position.z + 0.8f + floatOffset
    );
        
    // Hide icon when disabled or being repaired
    floatingIcon->active = !isDisabled && !guardIsRepairing;
}
    
// Update interaction prompt text AND enabled state
auto interactable = gameObject->GetComponent<Engine::Interactable>();
if (interactable) {
    // DISABLE interaction when terminal is offline, cooling down, or being repaired
    // ENABLE interaction only when terminal is active and ready
    bool canInteract = !isDisabled && cooldownTimer <= 0.0f && !guardIsRepairing;
    
    // Log only when interaction state changes
    static bool lastInteractState = true;
    if (canInteract != lastInteractState) {
        std::cout << "[CCTVTerminal] Zone " << controlledZone 
                  << " interaction " << (canInteract ? "ENABLED" : "DISABLED")
                  << " (isDisabled=" << isDisabled 
                  << " cooldown=" << cooldownTimer 
                  << " repairing=" << guardIsRepairing << ")\n";
        lastInteractState = canInteract;
    }
    
    interactable->enabled = canInteract;
    
    // Update text based on state
    if (isDisabled) {
        interactable->actionText = GetZoneName(controlledZone) + " CAMERAS OFFLINE";
    } else if (cooldownTimer > 0.0f) {
        interactable->actionText = "TERMINAL COOLING DOWN";
    } else if (guardIsRepairing) {
        interactable->actionText = "GUARD REPAIRING SYSTEM";
    } else {
        interactable->actionText = "Hold E to Hack " + GetZoneName(controlledZone) + " Cameras";
    }
}
    
    if (isDisabled) {
        UpdateDisabledState(Engine::Time::deltaTime);
    }
    
    if (cooldownTimer > 0.0f) {
        UpdateCooldown(Engine::Time::deltaTime);
    }
    
    if (guardIsRepairing) {
        UpdateRepair(Engine::Time::deltaTime);
    }
    
    UpdateVisualState();
}

void CCTVTerminal::DisableCameras() {
    std::cout << "[CCTVTerminal] Zone " << controlledZone << " cameras DISABLED for " << disabledDuration << " seconds\n";
    
    isDisabled = true;
    disabledTimer = disabledDuration;
    state = TerminalState::DISABLED;
    cooldownTimer = cooldownDuration;
    
    // Disable all cameras in this zone
    int disabledCount = 0;
    for (auto* camera : controlledCameras) {
        if (camera) {
            camera->isDisabled = true;
            camera->disabledTimeRemaining = disabledDuration;
            disabledCount++;
            std::cout << "[CCTVTerminal] Disabled camera ID " << camera->id << "\n";
        }
    }
    std::cout << "[CCTVTerminal] Disabled " << disabledCount << "/" << controlledCameras.size() << " cameras\n";
    
    // Notify guards
    std::cout << "[CCTVTerminal] Notifying " << allGuards.size() << " guards of sabotage\n";
    NotifyGuardsOfSabotage();
}

void CCTVTerminal::RestoreCameras() {
    std::cout << "[CCTVTerminal] Zone " << controlledZone << " cameras RESTORED\n";
    
    isDisabled = false;
    disabledTimer = 0.0f;
    state = TerminalState::ACTIVE;
    guardIsRepairing = false;
    repairingGuard = nullptr;
    repairProgress = 0.0f;
    
    // Restore all cameras in this zone
    for (auto* camera : controlledCameras) {
        if (camera) {
            camera->isDisabled = false;
            camera->disabledTimeRemaining = 0.0f;
        }
    }
}

void CCTVTerminal::StartRepair(SecurityGuard* guard) {
    if (!isDisabled) return;
    
    guardIsRepairing = true;
    repairingGuard = guard;
    repairProgress = 0.0f;
    state = TerminalState::REPAIRING;
    
    std::cout << "[CCTVTerminal] Guard started repairing Zone " << controlledZone << " terminal\n";
}

void CCTVTerminal::CancelRepair() {
    guardIsRepairing = false;
    repairingGuard = nullptr;
    repairProgress = 0.0f;
    state = TerminalState::DISABLED;
    
    std::cout << "[CCTVTerminal] Repair cancelled\n";
}

void CCTVTerminal::UpdateRepair(float deltaTime) {
    if (!guardIsRepairing || !repairingGuard) {
        CancelRepair();
        return;
    }
    
    // Check if guard is still nearby
    Engine::Vector2 guardPos(repairingGuard->transform->position.x, 
                             repairingGuard->transform->position.y);
    Engine::Vector2 terminalPos(transform->position.x, transform->position.y);
    float distance = Engine::Vector2::Distance(guardPos, terminalPos);
    
    if (distance > 2.0f) {
        // Guard moved away
        CancelRepair();
        return;
    }
    
    // Progress repair
    repairProgress += deltaTime / repairTime;
    
    if (repairProgress >= 1.0f) {
        // Repair complete!
        RestoreCameras();
    }
}

void CCTVTerminal::NotifyGuardsOfSabotage() {
    Engine::Vector2 terminalPos(transform->position.x, transform->position.y);
    
    std::cout << "[CCTVTerminal] Zone " << controlledZone << " sabotaged at (" 
              << terminalPos.x << ", " << terminalPos.y << ")\n";
    
    // ONLY notify the guard from this zone's home
    int notified = 0;
    for (auto* guard : allGuards) {
        if (!guard) continue;
        
        // Only the home zone guard should respond
        if (guard->homeZoneId == controlledZone) {
            std::cout << "[CCTVTerminal] Notifying home guard from Zone " 
                      << controlledZone << "\n";
            guard->OnCCTVSystemSabotaged(terminalPos, controlledZone);
            notified++;
            break; // Only one guard needed
        }
    }
    
    if (notified == 0) {
        std::cout << "[CCTVTerminal] WARNING: No home zone guard found for Zone " 
                  << controlledZone << "!\n";
    } else {
        std::cout << "[CCTVTerminal] Notified " << notified << " guard (home zone only)\n";
    }
}

void CCTVTerminal::UpdateDisabledState(float deltaTime) {
    disabledTimer -= deltaTime;
    
    if (disabledTimer <= 0.0f && !guardIsRepairing) {
        // Auto-restore if guard didn't repair
        RestoreCameras();
    }
}

void CCTVTerminal::UpdateCooldown(float deltaTime) {
    cooldownTimer -= deltaTime;
    if (cooldownTimer < 0.0f) {
        cooldownTimer = 0.0f;
    }
}

void CCTVTerminal::UpdateVisualState() {
    if (!spriteRenderer) return;
    
    Engine::Color color = GetStateColor();
    
    // Add pulsing effect for HACKING and REPAIRING states
    if (state == TerminalState::HACKING || state == TerminalState::REPAIRING) {
        float pulse = 0.5f + 0.5f * sinf(Engine::Time::time * 3.0f);
        color.a = pulse;
    }
    
    spriteRenderer->SetColor(color);
}

Engine::Color CCTVTerminal::GetStateColor() const {
    switch (state) {
        case TerminalState::ACTIVE:
            return Engine::Color(0.2f, 0.8f, 1.0f, 1.0f); // Cyan
        case TerminalState::HACKING:
            return Engine::Color(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
        case TerminalState::DISABLED:
            return Engine::Color(0.5f, 0.0f, 0.0f, 1.0f); // Dark red
        case TerminalState::REPAIRING:
            return Engine::Color(1.0f, 0.5f, 0.0f, 1.0f); // Orange
        default:
            return Engine::Color::white();
    }
}

} // namespace Game

