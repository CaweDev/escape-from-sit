/**
 * @file Door.cpp
 * @brief Implementation of Door component
 */
#include "Door.hpp"
#include "../Core/GameObject.hpp"
#include "../Core/Time.hpp"
#include "../Core/ConfigManager.hpp"
#include "../Physics/BoxCollider2D.hpp"
#include "../Interaction/Interactable.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "AEEngine.h"
#include "Engine/Audio/SoundManager.hpp"
#include "Engine/Graphics/Camera.hpp"
#include <algorithm>
#include <cstdio>

namespace Engine {

namespace {
    // Loaded from Assets/Config/Engine/door.json via Door::Initialize()
    static f32 s_sfxMaxAudibleDistance = 12.0f;
    static f32 s_passableThreshold     = 0.5f;
    static f32 s_colliderChangeThreshold = 0.1f;

    f32 ComputeDoorSfxVolumeScale(const Transform* doorTransform, int tileX, int tileY) {
        if (!Camera::main || !Camera::main->transform) {
            return 1.0f;
        }

        const Vector2 cameraPos(Camera::main->transform->position.x, Camera::main->transform->position.y);
        const Vector2 doorPos = doorTransform
            ? Vector2(doorTransform->position.x, doorTransform->position.y)
            : Vector2(static_cast<f32>(tileX) + 0.5f, static_cast<f32>(tileY) + 0.5f);

        const f32 distance = Vector2::Distance(cameraPos, doorPos);
        const f32 t = 1.0f - (distance / s_sfxMaxAudibleDistance);
        return std::clamp(t, 0.0f, 1.0f);
    }
}

void Door::Awake() {
    // Cache collider reference only if not already set by spawning logic
    if (!collider && gameObject) {
        collider = gameObject->GetComponent<BoxCollider2D>();
    }
    
    // Set initial collider size and offset based on facing
    if (collider) {
        collider->size = Vector2(1.0f, 1.0f);
    }

    
    UpdateCollider();
}

void Door::Update() {
	f32 dt = Time::deltaTime;
    
    // Handle animation
    switch (state) {
        case DoorState::Opening:
            openAmount += animationSpeed * dt;
            if (openAmount >= 1.0f) {
                openAmount = 1.0f;
                state = DoorState::Open;
                autoCloseTimer = 0.0f;
            }
            UpdateCollider();
            break;
            
        case DoorState::Closing:
            openAmount -= animationSpeed * dt;
            if (openAmount <= 0.0f) {
                openAmount = 0.0f;
                state = DoorState::Closed;
            }
            UpdateCollider();
            break;
            
        case DoorState::Open:
            // Handle auto-close
            if (!IsHeldOpen() && autoCloseDelay > 0.0f) {
                autoCloseTimer += dt;
                if (autoCloseTimer >= autoCloseDelay) {
                    Close();
                }
            }
            else {
                autoCloseTimer = 0.0f;
            }
            break;
            
        case DoorState::Closed:
            // Nothing to do
            break;
    }
    
    // Update interactable text if bound
    if (m_interactable) {
        if (state == DoorState::Closed || state == DoorState::Closing) {
            m_interactable->actionText = m_openText;
        } else {
            m_interactable->actionText = m_closeText;
        }
    }
}

void Door::Open() {
    if (state == DoorState::Closed) {
        state = DoorState::Opening;

        // Distance-based attenuation from the main camera to this door.
        // 1.0 at/near the door, fading linearly to 0.0 at max audible distance.
        const f32 volumeScale = ComputeDoorSfxVolumeScale(transform, tileX, tileY);
        SoundManager::PlaySFX("door_open", volumeScale);
    }
    else if (state == DoorState::Closing) {
        // Reopen silently from closing to avoid rapid open-sfx retrigger spam.
        state = DoorState::Opening;
    }
    autoCloseTimer = 0.0f;
}

void Door::Close() {
    if (state == DoorState::Open || state == DoorState::Opening) {
        state = DoorState::Closing;

        // Distance-based attenuation from the main camera to this door.
        // 1.0 at/near the door, fading linearly to 0.0 at max audible distance.
        const f32 volumeScale = ComputeDoorSfxVolumeScale(transform, tileX, tileY);
        SoundManager::PlaySFX("door_close", volumeScale);
    }
}

void Door::Toggle() {
    switch (state) {
        case DoorState::Closed:
        case DoorState::Closing:
            Open();
            break;
        case DoorState::Open:
        case DoorState::Opening:
            Close();
            break;
    }
}

bool Door::IsPassable() const {
    // Door is passable when open beyond the configured threshold
    return openAmount > s_passableThreshold;
}

void Door::UpdateCollider() {
    if (!collider && gameObject) {
        collider = gameObject->GetComponent<BoxCollider2D>();
    }
    
    if (collider) {
        bool shouldBlock = !this->IsPassable();
        collider->enabled = shouldBlock;
        
        // Debug output only when door state changes (not every frame)
        static float lastOpenAmount = -1.0f;
        if (std::abs(openAmount - lastOpenAmount) > s_colliderChangeThreshold) {
            lastOpenAmount = openAmount;
            // Only log significant changes
            // printf("[Door] openAmount=%.2f, passable=%s, collider=%s\n", 
            //        openAmount, 
            //        IsPassable() ? "YES" : "NO",
            //        collider->enabled ? "BLOCKING" : "OPEN");
        }
    }
}

Vector2 Door::GetWorldPosition() const {
    return Vector2(static_cast<f32>(tileX) + 0.5f, static_cast<f32>(tileY) + 0.5f);
}

void Door::SetupInteractable(Interactable* interactable, const char* openText, const char* closeText) {
    if (!interactable) return;
    
    m_interactable = interactable;
    m_openText = openText;
    m_closeText = closeText;
    
    // Set initial text
    interactable->actionText = (state == DoorState::Closed) ? openText : closeText;
    
    // Add interaction callback
    interactable->onInteract = [this]() {
        this->Toggle();
    };
}

void Door::DisableInteraction() {
    // Clear the interaction callback so Door doesn't respond to interactions
    // This allows other components (like ControlRoomDoor) to take full control
    if (m_interactable) {
        m_interactable->onInteract = nullptr;
        m_interactable = nullptr;
    }
}

void Door::RequestOpen(const void* requester)
{
    if (!requester) {
        return;
   }

    const auto [it, inserted] = m_holders.insert(requester);
    (void)it;

    // Always refresh hold timer while requested.
    autoCloseTimer = 0.0f;

    // New holder: request open normally.
    if (inserted) {
        Open();
        return;
    }

    // Existing holder: if door was externally forced shut/closing,
    // reassert opening so AI holders don't get stuck behind it.
    if (state == DoorState::Closed || state == DoorState::Closing) {
        Open();
    }
}

void Door::ReleaseOpen(const void* requester)
{
    if (!requester) {
        return;
    }

    const size_t removed = m_holders.erase(requester);

    // Ignore duplicate/invalid releases from non-holders.
    if (removed == 0) {
        return;
    }

    if (!IsHeldOpen()) {
        autoCloseTimer = 0.0f;
        Close();
    }
}

void Door::Initialize(const std::string& configPath)
{
    DoorConfig config;
    if (!ConfigManager::ReadConfig(configPath, config))
    {
        std::printf("[Warning] Door: Failed to read config '%s'. Using built-in defaults.\n",
                    configPath.c_str());
    }
    s_sfxMaxAudibleDistance   = config.sfxMaxAudibleDistance;
    s_passableThreshold       = config.passableThreshold;
    s_colliderChangeThreshold = config.colliderChangeThreshold;
    // animationSpeed is a per-instance member; the config default can be applied
    // to new doors by reading DoorConfig::animationSpeed wherever doors are spawned.
}

Door* Door::FindAtTile(int tileX, int tileY)
{
    Scene* scene = SceneManager::GetActiveScene();
    if (!scene)
    {
        return nullptr;
    }

    for (auto& obj : scene->GetGameObjects())
    {
        if (!obj || !obj->active)
        {
            continue;
        }

        Door* door = obj->GetComponent<Door>();
        if (!door)
        {
            continue;
        }

        if (door->tileX == tileX && door->tileY == tileY)
        {
            return door;
        }
    }

    return nullptr;
}

} // namespace Engine
