/**
 * @file InteractionController.cpp
 * @brief Implementation of InteractionController component
 */
#include "InteractionController.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/Raycasting/ProjectionHelper.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/UI/UIManager.hpp"
#include "Engine/UI/Crosshair.hpp"
#include "Engine/UI/InteractionPrompt.hpp"
#include "Engine/Graphics/DoorRenderer.hpp"
#include "Engine/TileMap/Door.hpp"
#include "Engine/Math/Mathf.hpp"
#include "Engine/Core/AssetManager.hpp"
#include <cfloat>
#include <cmath>

namespace Engine {

void InteractionController::Initialize() {
    // Create UI canvas for interaction elements
    m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
    
    // Set default textures
	defaultCrosshairTexture = AssetManager::GetTexture("Assets/UI/crosshair_default.png");
	highlightCrosshairTexture = AssetManager::GetTexture("Assets/UI/crosshair_highlight.png");

    // Create crosshair (centered on screen)
    m_crosshair = m_canvas->CreateChild<UI::Crosshair>();
    m_crosshair->SetDefaultTexture(defaultCrosshairTexture);
    m_crosshair->SetHighlightTexture(highlightCrosshairTexture);
    
    // Create interaction prompt (below crosshair)
    m_prompt = m_canvas->CreateChild<UI::InteractionPrompt>();
    m_prompt->Hide();
}

void InteractionController::Update() {
    // Only run if this is the main camera's gameObject
    if (!IsActiveCamera()) {
        return;
    }
    
    // Update crosshair textures if changed at runtime
    if (m_crosshair) {
        m_crosshair->SetDefaultTexture(defaultCrosshairTexture);
        m_crosshair->SetHighlightTexture(highlightCrosshairTexture);
    }
    
    // Find what we're looking at
    Interactable* target = FindTargetAtScreenCenter();
    
    // Update if target changed
    if (target != m_currentTarget) {
        SetTarget(target);
    }
    
    // Always refresh the prompt text in case it changed (e.g., door Open/Close)
    if (m_currentTarget && m_prompt) {
        m_prompt->Show(m_currentTarget->actionText);
    }
    
    // Handle interaction input
    if (m_currentTarget && Input::GetKeyDown(interactKey)) {
        m_currentTarget->Interact();
    }
}

void InteractionController::OnDestroy() {
    // Clear highlight on current target
    if (m_currentTargetRenderer) {
        m_currentTargetRenderer->highlighted = false;
    }
    
    // Clean up UI (check if UIManager still exists during shutdown)
    if (m_canvas && UI::UIManager::GetInstance()) {
        UI::UIManager::GetInstance()->DestroyCanvas(m_canvas);
    }
    m_canvas = nullptr;
    m_crosshair = nullptr;
    m_prompt = nullptr;
}

bool InteractionController::IsActiveCamera() const {
    return Camera::main && Camera::main->gameObject == gameObject;
}

Interactable* InteractionController::FindTargetAtScreenCenter() {
    Scene* scene = SceneManager::GetActiveScene();
    if (!scene || !Camera::main) return nullptr;
    
    s64 sw = Application::GetWindowWidth();
    s64 sh = Application::GetWindowHeight();
    f32 screenCenterX = static_cast<f32>(sw) / 2.0f;
    f32 screenCenterY = static_cast<f32>(sh) / 2.0f;
    
    Interactable* closestTarget = nullptr;
    f32 closestDist = FLT_MAX;
    
    // Loop through all GameObjects in the scene
    for (auto& obj : scene->GetGameObjects()) {
        if (!obj->active) continue;
        
        // Check if it has an Interactable component
        Interactable* interactable = obj->GetComponent<Interactable>();
        if (!interactable || !interactable->enabled) continue;
        
        bool isHit = false;
        f32 perpDistance = FLT_MAX;

        // Try SpriteRenderer (billboard)
        SpriteRenderer* spriteRenderer = obj->GetComponent<SpriteRenderer>();
        if (spriteRenderer && spriteRenderer->billboard && spriteRenderer->sprite) {
            // Project to screen
            ProjectionResult proj = ProjectionHelper::ProjectToScreen(
                obj->transform->position,
                Camera::main->transform->position,
                Camera::main->transform->rotation,
                Camera::main->fov,
                sw, sh
            );
            
            if (proj.isVisible && proj.perpDistance <= interactable->maxDistance) {
                // Calculate screen bounds of the sprite
                f32 spriteHeight = proj.scale * obj->transform->localScale.y;
                f32 spriteWidth = proj.scale * obj->transform->localScale.x;
                
                // Screen bounds (screenY is at the sprite's bottom)
                f32 left = proj.screenX - spriteWidth / 2.0f;
                f32 right = proj.screenX + spriteWidth / 2.0f;
                f32 bottom = proj.screenY;
                f32 top = proj.screenY - spriteHeight;
                
                // Calculate center of sprite on screen
                f32 spriteCenterX = proj.screenX;
                f32 spriteCenterY = proj.screenY - spriteHeight / 2.0f;
                
                if (interactable->hitShape == InteractableHitShape::Ellipse) {
                    f32 a = spriteWidth / 2.0f;
                    f32 b = spriteHeight / 2.0f;
                    f32 dx = screenCenterX - spriteCenterX;
                    f32 dy = screenCenterY - spriteCenterY;
                    if ((dx * dx) / (a * a) + (dy * dy) / (b * b) <= 1.0f) {
                        isHit = true;
                        perpDistance = proj.perpDistance;
                    }
                } else {
                    if (screenCenterX >= left && screenCenterX <= right &&
                        screenCenterY >= top && screenCenterY <= bottom) {
                        isHit = true;
                        perpDistance = proj.perpDistance;
                    }
                }
            }
        }

        // Try DoorRenderer (axis-aligned segment)
        if (!isHit) {
            DoorRenderer* doorRenderer = obj->GetComponent<DoorRenderer>();
            if (doorRenderer) {
                Door* door = obj->GetComponent<Door>();
                if (door) {
                    Vector3 camPos = Camera::main->transform->position;
                    f32 camAngle = Camera::main->transform->rotation * Mathf::Deg2Rad;
                    f32 dirX = cosf(camAngle);
                    f32 dirY = sinf(camAngle);

                    // Use the door's base tile position for interaction detection
                    // This allows interaction even when the door is slid open
                    f32 doorCenterX = static_cast<f32>(door->tileX) + 0.5f;
                    f32 doorCenterY = static_cast<f32>(door->tileY) + 0.5f;
                    
                    // Calculate distance to door center
                    f32 dx = doorCenterX - camPos.x;
                    f32 dy = doorCenterY - camPos.y;
                    f32 distToDoor = sqrtf(dx * dx + dy * dy);
                    
                    // Check if we're close enough
                    if (distToDoor <= interactable->maxDistance) {
                        // Check if we're looking towards the door (within ~45 degrees)
                        f32 dotTowardsDoor = (dx * dirX + dy * dirY);
                        if (dotTowardsDoor > 0) { // Door is in front of us
                            // Normalize and check angle
                            f32 normDot = dotTowardsDoor / distToDoor;
                            if (normDot > 0.7f) { // roughly looking at it
                                isHit = true;
                                perpDistance = distToDoor;
                            }
                        }
                    }
                }
            }
        }
        
        if (isHit) {
            // Pick the closest one
            if (perpDistance < closestDist) {
                closestDist = perpDistance;
                closestTarget = interactable;
            }
        }
    }
    
    return closestTarget;
}

void InteractionController::SetTarget(Interactable* target) {
    // Clear highlight on previous target
    if (m_currentTargetRenderer) {
        m_currentTargetRenderer->highlighted = false;
    }
    
    m_currentTarget = target;
    
    if (target) {
        // Set new target renderer - try both SpriteRenderer and DoorRenderer via IRenderer
        m_currentTargetRenderer = target->gameObject->GetComponent<IRenderer>();
        if (m_currentTargetRenderer) {
            m_currentTargetRenderer->highlighted = true;
        }
        
        // Update UI
        if (m_crosshair) {
            m_crosshair->SetHighlighted(true);
        }
        if (m_prompt) {
            m_prompt->Show(target->actionText);
        }
    } else {
        // No target
        m_currentTargetRenderer = nullptr;
        
        if (m_crosshair) {
            m_crosshair->SetHighlighted(false);
        }
        if (m_prompt) {
            m_prompt->Hide();
        }
    }
}

} // namespace Engine
