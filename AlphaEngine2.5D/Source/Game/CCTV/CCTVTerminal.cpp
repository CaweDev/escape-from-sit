/*!
 * \file CCTVTerminal.cpp
 * \brief Component that represents an interactive CCTV terminal in the world
 */

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

namespace Game
{

    /*!
     * \brief [AUTO] Awake
     */
    void CCTVTerminal::Awake()
    {
        UpdateVisualState();
        SetupInteraction();
    }

    /*!
     * \brief [AUTO] SetupInteraction
     */
    void CCTVTerminal::SetupInteraction()
    {
        auto interactable = gameObject->GetComponent<Engine::Interactable>();
        if (interactable)
        {
            interactable->onInteract = [this]()
            { OnTerminalInteract(); };
        }
    }

    /*!
     * \brief [AUTO] OnTerminalInteract
     */
    void CCTVTerminal::OnTerminalInteract()
    {
        if (!CanHack())
        {
            return;
        }

        Engine::SoundManager::PlaySFX("terminal_hack");

        // For now, instant hack
        DisableCameras();
        Engine::SoundManager::PlaySFX("cctv_disabled");

        if (playerHUD)
        {
            char zoneLetter = ZoneIdToLetter(controlledZone);
            std::string message = std::string("ZONE ") + zoneLetter + hackStatusPrefix;
            playerHUD->ShowHackStatus(message, hackStatusDuration);
        }
    }

    /*!
     * \brief [AUTO] CanHack
     * \return
     */
    bool CCTVTerminal::CanHack() const
    {
        if (isDisabled)
            return false;
        if (cooldownTimer > 0.0f)
            return false;
        if (guardIsRepairing)
            return false;
        return true;
    }

    /*!
     * \brief [AUTO] Update
     */
    void CCTVTerminal::Update()
    {
        if (floatingIcon)
        {
            floatAnimTime += Engine::Time::deltaTime;
            float floatOffset = std::sin(floatAnimTime * floatSpeed) * floatAmplitude;
            floatingIcon->transform->position = Engine::Vector3(
                transform->position.x,
                transform->position.y,
                transform->position.z + floatBaseHeightOffset + floatOffset);

            floatingIcon->active = !isDisabled && !guardIsRepairing;
        }

        auto interactable = gameObject->GetComponent<Engine::Interactable>();
        if (interactable)
        {
            bool canInteract = !isDisabled && cooldownTimer <= 0.0f && !guardIsRepairing;
            interactable->enabled = canInteract;

            if (isDisabled)
            {
                interactable->actionText = GetZoneName(controlledZone) + disabledTextSuffix;
            }
            else if (cooldownTimer > 0.0f)
            {
                interactable->actionText = cooldownText;
            }
            else if (guardIsRepairing)
            {
                interactable->actionText = repairingText;
            }
            else
            {
                interactable->actionText = zoneHackTextPrefix + GetZoneName(controlledZone) + zoneHackTextSuffix;
            }
        }

        if (isDisabled)
        {
            UpdateDisabledState(Engine::Time::deltaTime);
        }

        if (cooldownTimer > 0.0f)
        {
            UpdateCooldown(Engine::Time::deltaTime);
        }

        if (guardIsRepairing)
        {
            UpdateRepair(Engine::Time::deltaTime);
        }

        UpdateVisualState();
    }

    /*!
     * \brief [AUTO] DisableCameras
     */
    void CCTVTerminal::DisableCameras()
    {
        isDisabled = true;
        disabledTimer = disabledDuration;
        state = TerminalState::DISABLED;
        cooldownTimer = cooldownDuration;

        for (auto *camera : controlledCameras)
        {
            if (camera)
            {
                camera->isDisabled = true;
                camera->disabledTimeRemaining = disabledDuration;
            }
        }

        NotifyGuardsOfSabotage();
    }

    /*!
     * \brief [AUTO] RestoreCameras
     */
    void CCTVTerminal::RestoreCameras()
    {
        isDisabled = false;
        disabledTimer = 0.0f;
        state = TerminalState::ACTIVE;
        guardIsRepairing = false;
        repairingGuard = nullptr;
        repairProgress = 0.0f;

        for (auto *camera : controlledCameras)
        {
            if (camera)
            {
                camera->isDisabled = false;
                camera->disabledTimeRemaining = 0.0f;
            }
        }
    }

    /*!
     * \brief [AUTO] StartRepair
     * \param guard [in]
     */
    void CCTVTerminal::StartRepair(SecurityGuard *guard)
    {
        if (!isDisabled)
            return;

        guardIsRepairing = true;
        repairingGuard = guard;
        repairProgress = 0.0f;
        state = TerminalState::REPAIRING;
    }

    /*!
     * \brief [AUTO] CancelRepair
     */
    void CCTVTerminal::CancelRepair()
    {
        guardIsRepairing = false;
        repairingGuard = nullptr;
        repairProgress = 0.0f;
        state = TerminalState::DISABLED;
    }

    /*!
     * \brief [AUTO] UpdateRepair
     * \param deltaTime [in]
     */
    void CCTVTerminal::UpdateRepair(float deltaTime)
    {
        if (!guardIsRepairing || !repairingGuard)
        {
            CancelRepair();
            return;
        }

        Engine::Vector2 guardPos(repairingGuard->transform->position.x,
                                 repairingGuard->transform->position.y);
        Engine::Vector2 terminalPos(transform->position.x, transform->position.y);
        float distance = Engine::Vector2::Distance(guardPos, terminalPos);

        if (distance > repairMaxDistance)
        {
            CancelRepair();
            return;
        }

        repairProgress += deltaTime / repairTime;

        if (repairProgress >= 1.0f)
        {
            RestoreCameras();
        }
    }

    /*!
     * \brief [AUTO] NotifyGuardsOfSabotage
     */
    void CCTVTerminal::NotifyGuardsOfSabotage()
    {
        Engine::Vector2 terminalPos(transform->position.x, transform->position.y);

        for (auto *guard : allGuards)
        {
            if (!guard)
                continue;

            if (guard->homeZoneId == controlledZone)
            {
                guard->OnCCTVSystemSabotaged(terminalPos, controlledZone);
                break;
            }
        }
    }

    /*!
     * \brief [AUTO] UpdateDisabledState
     * \param deltaTime [in]
     */
    void CCTVTerminal::UpdateDisabledState(float deltaTime)
    {
        disabledTimer -= deltaTime;

        if (disabledTimer <= 0.0f && !guardIsRepairing)
        {
            RestoreCameras();
        }
    }

    /*!
     * \brief [AUTO] UpdateCooldown
     * \param deltaTime [in]
     */
    void CCTVTerminal::UpdateCooldown(float deltaTime)
    {
        cooldownTimer -= deltaTime;
        if (cooldownTimer < 0.0f)
        {
            cooldownTimer = 0.0f;
        }
    }

    /*!
     * \brief [AUTO] UpdateVisualState
     */
    void CCTVTerminal::UpdateVisualState()
    {
        if (!spriteRenderer)
            return;

        Engine::Color color = GetStateColor();

        if (state == TerminalState::HACKING || state == TerminalState::REPAIRING)
        {
            float pulse = 0.5f + 0.5f * std::sin(Engine::Time::time * pulseSpeed);
            color.a = pulse;
        }

        spriteRenderer->SetColor(color);
    }

    /*!
     * \brief [AUTO] GetStateColor
     * \return
     */
    Engine::Color CCTVTerminal::GetStateColor() const
    {
        switch (state)
        {
        case TerminalState::ACTIVE:
            return activeColor;
        case TerminalState::HACKING:
            return hackingColor;
        case TerminalState::DISABLED:
            return disabledColor;
        case TerminalState::REPAIRING:
            return repairingColor;
        default:
            return Engine::Color::white();
        }
    }

} // namespace Game