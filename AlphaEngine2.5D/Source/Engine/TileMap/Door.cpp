/**
 * @file Door.cpp
 * @brief Implementation of Door component
 */
#include "Door.hpp"
#include "../Core/GameObject.hpp"
#include "../Physics/BoxCollider2D.hpp"
#include "../Interaction/Interactable.hpp"
#include "AEEngine.h"

namespace Engine {

void Door::Awake() {
    // Cache collider reference only if not already set by spawning logic
    if (!collider && gameObject) {
        collider = gameObject->GetComponent<BoxCollider2D>();
    }
    
    // Set initial collider size and offset based on facing
    if (collider) {
        if (facing == DoorFacing::NorthSouth) {
            // Door runs East-West, blocks North-South. Thin in Y.
            collider->size = Vector2(1.0f, 0.4f);
            collider->offset = Vector2(0, 0);
        } else {
            // Door runs North-South, blocks East-West. Thin in X.
            collider->size = Vector2(0.4f, 1.0f);
            collider->offset = Vector2(0, 0);
        }
    }
    
    UpdateCollider();
}

void Door::Update() {
    f32 dt = static_cast<f32>(AEFrameRateControllerGetFrameTime());
    
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
            if (autoCloseDelay > 0.0f) {
                autoCloseTimer += dt;
                if (autoCloseTimer >= autoCloseDelay) {
                    Close();
                }
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
    if (state == DoorState::Closed || state == DoorState::Closing) {
        state = DoorState::Opening;
    }
}

void Door::Close() {
    if (state == DoorState::Open || state == DoorState::Opening) {
        state = DoorState::Closing;
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
    // Semi-open doors are still collidable for simplicity, or we can use a threshold
    return openAmount > 0.8f; 
}

void Door::UpdateCollider() {
    if (!collider && gameObject) {
        collider = gameObject->GetComponent<BoxCollider2D>();
    }
    
    if (collider) {
        collider->enabled = !this->IsPassable();
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

} // namespace Engine
