#include "CCTVTerminal.hpp"
#include "CCTVCamera.hpp"
#include "Game/SecurityGuard/SecurityGuard.hpp"
#include "Game/Scenes/FPSScene/UI/PlayerHUD.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Interaction/Interactable.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/Audio/SoundManager.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <iostream>
#include <cmath>

namespace
{
    struct CCTVTerminalConfig : public Engine::IConfig
    {
        float interactMaxDistance = 2.5f;
        float holdDuration = 3.0f;
        float disabledDuration = 30.0f;
        float repairTime = 5.0f;
        float cooldownDuration = 10.0f;
        float hackStatusDuration = 3.0f;
        float repairMaxDistance = 2.0f;

        float floatSpeed = 2.0f;
        float floatAmplitude = 0.15f;
        float floatBaseHeightOffset = 0.8f;
        float pulseSpeed = 3.0f;

        Engine::Color activeColor = Engine::Color(0.2f, 0.8f, 1.0f, 1.0f);
        Engine::Color hackingColor = Engine::Color(1.0f, 1.0f, 0.0f, 1.0f);
        Engine::Color disabledColor = Engine::Color(0.5f, 0.0f, 0.0f, 1.0f);
        Engine::Color repairingColor = Engine::Color(1.0f, 0.5f, 0.0f, 1.0f);

        std::string interactText = "Hold E to Hack Terminal";
        std::string zoneHackTextPrefix = "Hold E to Hack ";
        std::string zoneHackTextSuffix = " Cameras";
        std::string disabledTextSuffix = " CAMERAS OFFLINE";
        std::string cooldownText = "TERMINAL COOLING DOWN";
        std::string repairingText = "GUARD REPAIRING SYSTEM";
        std::string hackStatusPrefix = " CAMERAS DISABLED FOR 30s";

        static Engine::Color ParseColor(const Json::Value& j, const char* key, const Engine::Color& fallback)
        {
            if (!j.isMember(key) || !j[key].isObject()) return fallback;
            const auto& c = j[key];
            return Engine::Color(
                c.get("r", fallback.r).asFloat(),
                c.get("g", fallback.g).asFloat(),
                c.get("b", fallback.b).asFloat(),
                c.get("a", fallback.a).asFloat());
        }

        void FromJson(const Json::Value& j) override
        {
            interactMaxDistance = j.get("interactMaxDistance", interactMaxDistance).asFloat();
            holdDuration = j.get("holdDuration", holdDuration).asFloat();
            disabledDuration = j.get("disabledDuration", disabledDuration).asFloat();
            repairTime = j.get("repairTime", repairTime).asFloat();
            cooldownDuration = j.get("cooldownDuration", cooldownDuration).asFloat();
            hackStatusDuration = j.get("hackStatusDuration", hackStatusDuration).asFloat();
            repairMaxDistance = j.get("repairMaxDistance", repairMaxDistance).asFloat();

            floatSpeed = j.get("floatSpeed", floatSpeed).asFloat();
            floatAmplitude = j.get("floatAmplitude", floatAmplitude).asFloat();
            floatBaseHeightOffset = j.get("floatBaseHeightOffset", floatBaseHeightOffset).asFloat();
            pulseSpeed = j.get("pulseSpeed", pulseSpeed).asFloat();

            activeColor = ParseColor(j, "activeColor", activeColor);
            hackingColor = ParseColor(j, "hackingColor", hackingColor);
            disabledColor = ParseColor(j, "disabledColor", disabledColor);
            repairingColor = ParseColor(j, "repairingColor", repairingColor);

            interactText = j.get("interactText", interactText).asString();
            zoneHackTextPrefix = j.get("zoneHackTextPrefix", zoneHackTextPrefix).asString();
            zoneHackTextSuffix = j.get("zoneHackTextSuffix", zoneHackTextSuffix).asString();
            disabledTextSuffix = j.get("disabledTextSuffix", disabledTextSuffix).asString();
            cooldownText = j.get("cooldownText", cooldownText).asString();
            repairingText = j.get("repairingText", repairingText).asString();
            hackStatusPrefix = j.get("hackStatusPrefix", hackStatusPrefix).asString();
        }
    };

    CCTVTerminalConfig g_cctvTerminalConfig;
    bool g_cctvTerminalConfigLoaded = false;

    const CCTVTerminalConfig& GetCCTVTerminalConfig()
    {
        if (!g_cctvTerminalConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/cctv_terminal.json", g_cctvTerminalConfig);
            if (g_cctvTerminalConfig.interactMaxDistance < 0.1f) g_cctvTerminalConfig.interactMaxDistance = 0.1f;
            if (g_cctvTerminalConfig.holdDuration < 0.01f) g_cctvTerminalConfig.holdDuration = 0.01f;
            if (g_cctvTerminalConfig.disabledDuration < 0.0f) g_cctvTerminalConfig.disabledDuration = 0.0f;
            if (g_cctvTerminalConfig.repairTime < 0.01f) g_cctvTerminalConfig.repairTime = 0.01f;
            if (g_cctvTerminalConfig.cooldownDuration < 0.0f) g_cctvTerminalConfig.cooldownDuration = 0.0f;
            if (g_cctvTerminalConfig.hackStatusDuration < 0.0f) g_cctvTerminalConfig.hackStatusDuration = 0.0f;
            if (g_cctvTerminalConfig.repairMaxDistance < 0.1f) g_cctvTerminalConfig.repairMaxDistance = 0.1f;
            g_cctvTerminalConfigLoaded = true;
        }
        return g_cctvTerminalConfig;
    }
}

namespace Game {

void CCTVTerminal::Awake() {
    const auto& cfg = GetCCTVTerminalConfig();
    disabledDuration = cfg.disabledDuration;
    repairTime = cfg.repairTime;
    cooldownDuration = cfg.cooldownDuration;
    interactionTime = cfg.holdDuration;

    //std::cout << "[CCTVTerminal::Awake] Terminal spawned for Zone " << controlledZone << "\n";
    

    UpdateVisualState();
    SetupInteraction();
}

void CCTVTerminal::SetupInteraction() {
    //std::cout << "[CCTVTerminal::SetupInteraction] Setting up interaction for Zone " << controlledZone << "\n";
    const auto& cfg = GetCCTVTerminalConfig();
    
    // Add Interactable component
    auto interactable = gameObject->GetComponent<Engine::Interactable>();
    if (!interactable) {
        interactable = gameObject->AddComponent<Engine::Interactable>();
        //std::cout << "[CCTVTerminal::SetupInteraction] Created new Interactable component\n";
    } else {
        //std::cout << "[CCTVTerminal::SetupInteraction] Using existing Interactable component\n";
    }
    
    interactable->actionText = cfg.interactText;
    interactable->maxDistance = cfg.interactMaxDistance;
    interactable->requireHold = true;
    interactable->holdDuration = cfg.holdDuration;
    interactable->onInteract = [this]() { OnTerminalInteract(); };
    
    //std::cout << "[CCTVTerminal::SetupInteraction] Interaction setup complete!\n";
}

void CCTVTerminal::OnTerminalInteract() {
    const auto& cfg = GetCCTVTerminalConfig();
    /*std::cout << "[CCTVTerminal] OnTerminalInteract called! Zone=" << controlledZone 
              << " (" << ZoneIdToLetter(controlledZone) << ")"
              << " isDisabled=" << isDisabled << " cooldown=" << cooldownTimer 
              << " playerHUD=" << (playerHUD ? "YES" : "NULL") << "\n";*/
              
    if (!CanHack()) {
        //std::cout << "[CCTVTerminal] CanHack returned false!\n";
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
        std::string message = std::string("ZONE ") + zoneLetter + cfg.hackStatusPrefix;
        //std::cout << "[CCTVTerminal] Showing HUD message: " << message << "\n";
        playerHUD->ShowHackStatus(message, cfg.hackStatusDuration);
    } else {
        //std::cout << "[CCTVTerminal] playerHUD is NULL! Message not shown!\n";
    }
}

bool CCTVTerminal::CanHack() const {
    if (isDisabled) return false;
    if (cooldownTimer > 0.0f) return false;
    if (guardIsRepairing) return false;
    return true;
}

void CCTVTerminal::Update() {
const auto& cfg = GetCCTVTerminalConfig();
// Update floating icon animation
if (floatingIcon) {
    floatAnimTime += Engine::Time::deltaTime;
    float floatOffset = std::sin(floatAnimTime * cfg.floatSpeed) * cfg.floatAmplitude;
    floatingIcon->transform->position = Engine::Vector3(
        transform->position.x,
        transform->position.y,
        transform->position.z + cfg.floatBaseHeightOffset + floatOffset
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
        /*std::cout << "[CCTVTerminal] Zone " << controlledZone 
                  << " interaction " << (canInteract ? "ENABLED" : "DISABLED")
                  << " (isDisabled=" << isDisabled 
                  << " cooldown=" << cooldownTimer 
                  << " repairing=" << guardIsRepairing << ")\n";*/
        lastInteractState = canInteract;
    }
    
    interactable->enabled = canInteract;
    
    // Update text based on state
    if (isDisabled) {
        interactable->actionText = GetZoneName(controlledZone) + cfg.disabledTextSuffix;
    } else if (cooldownTimer > 0.0f) {
        interactable->actionText = cfg.cooldownText;
    } else if (guardIsRepairing) {
        interactable->actionText = cfg.repairingText;
    } else {
        interactable->actionText = cfg.zoneHackTextPrefix + GetZoneName(controlledZone) + cfg.zoneHackTextSuffix;
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
    //std::cout << "[CCTVTerminal] Zone " << controlledZone << " cameras DISABLED for " << disabledDuration << " seconds\n";
    
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
            //std::cout << "[CCTVTerminal] Disabled camera ID " << camera->id << "\n";
        }
    }
    //std::cout << "[CCTVTerminal] Disabled " << disabledCount << "/" << controlledCameras.size() << " cameras\n";
    
    // Notify guards
    //std::cout << "[CCTVTerminal] Notifying " << allGuards.size() << " guards of sabotage\n";
    NotifyGuardsOfSabotage();
}

void CCTVTerminal::RestoreCameras() {
    //std::cout << "[CCTVTerminal] Zone " << controlledZone << " cameras RESTORED\n";
    
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
    
    //std::cout << "[CCTVTerminal] Guard started repairing Zone " << controlledZone << " terminal\n";
}

void CCTVTerminal::CancelRepair() {
    guardIsRepairing = false;
    repairingGuard = nullptr;
    repairProgress = 0.0f;
    state = TerminalState::DISABLED;
    
    //std::cout << "[CCTVTerminal] Repair cancelled\n";
}

void CCTVTerminal::UpdateRepair(float deltaTime) {
    const auto& cfg = GetCCTVTerminalConfig();
    if (!guardIsRepairing || !repairingGuard) {
        CancelRepair();
        return;
    }
    
    // Check if guard is still nearby
    Engine::Vector2 guardPos(repairingGuard->transform->position.x, 
                             repairingGuard->transform->position.y);
    Engine::Vector2 terminalPos(transform->position.x, transform->position.y);
    float distance = Engine::Vector2::Distance(guardPos, terminalPos);
    
    if (distance > cfg.repairMaxDistance) {
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
    
    //std::cout << "[CCTVTerminal] Zone " << controlledZone << " sabotaged at (" 
    //          << terminalPos.x << ", " << terminalPos.y << ")\n";
    
    // ONLY notify the guard from this zone's home
    int notified = 0;
    for (auto* guard : allGuards) {
        if (!guard) continue;
        
        // Only the home zone guard should respond
        if (guard->homeZoneId == controlledZone) {
            /*std::cout << "[CCTVTerminal] Notifying home guard from Zone " 
                      << controlledZone << "\n";*/
            guard->OnCCTVSystemSabotaged(terminalPos, controlledZone);
            notified++;
            break; // Only one guard needed
        }
    }
    
   /* if (notified == 0) {
        std::cout << "[CCTVTerminal] WARNING: No home zone guard found for Zone " 
                  << controlledZone << "!\n";
    } else {
        std::cout << "[CCTVTerminal] Notified " << notified << " guard (home zone only)\n";
    }*/
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
    const auto& cfg = GetCCTVTerminalConfig();
    
    Engine::Color color = GetStateColor();
    
    // Add pulsing effect for HACKING and REPAIRING states
    if (state == TerminalState::HACKING || state == TerminalState::REPAIRING) {
        float pulse = 0.5f + 0.5f * sinf(Engine::Time::time * cfg.pulseSpeed);
        color.a = pulse;
    }
    
    spriteRenderer->SetColor(color);
}

Engine::Color CCTVTerminal::GetStateColor() const {
    const auto& cfg = GetCCTVTerminalConfig();
    switch (state) {
        case TerminalState::ACTIVE:
            return cfg.activeColor;
        case TerminalState::HACKING:
            return cfg.hackingColor;
        case TerminalState::DISABLED:
            return cfg.disabledColor;
        case TerminalState::REPAIRING:
            return cfg.repairingColor;
        default:
            return Engine::Color::white();
    }
}

} // namespace Game

