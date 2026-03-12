/**
 * @file InteractionController.cpp
 * @brief Implementation of InteractionController component
 */
#include "InteractionController.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Graphics/SpriteSheetRenderer.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/Raycasting/ProjectionHelper.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Input/XboxController.hpp"
#include "Engine/UI/Core/UIManager.hpp"
#include "Engine/UI/Specialized/Crosshair.hpp"
#include "Engine/UI/Specialized/InteractionPrompt.hpp"
#include "Engine/Graphics/DoorRenderer.hpp"
#include "Engine/TileMap/Door.hpp"
#include "Engine/Math/Mathf.hpp"
#include "Engine/Core/AssetManager.hpp"
#include <cfloat>
#include <cmath>
#include <string>

namespace Engine {

void InteractionController::Initialize() {
    // Create UI canvas for interaction elements
    m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
    m_canvas->SetSortOrder(50);
    
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
        if (m_currentTarget->requireHold) {
            // Show hold progress
            if (m_isHolding) {
                float progress = (m_currentTarget->holdDuration > 0) ? (m_holdTimer / m_currentTarget->holdDuration) : 0.0f;
                int percent = static_cast<int>(progress * 100.0f);
                std::string text = m_currentTarget->actionText + " [" + std::to_string(percent) + "%]";
                m_prompt->Show(text);
            } else {
                m_prompt->Show(m_currentTarget->actionText);
            }
        } else {
            m_prompt->Show(m_currentTarget->actionText);
        }
    }
    
    // Handle interaction input
    if (m_currentTarget) {
        if (m_currentTarget->requireHold) {
            // Hold-to-interact mechanic (E key, Left Click, or Xbox X button)
            if (Input::GetKey(interactKey) || Input::GetMouseButtonDown(0) || XboxController::GetButton(XboxButton::X, 0)) {
                if (!m_isHolding) {
                    m_isHolding = true;
                    m_holdTimer = 0.0f;
                }
                
                m_holdTimer += Time::deltaTime;
                
                if (m_holdTimer >= m_currentTarget->holdDuration) {
                    // Interaction complete
                    m_currentTarget->Interact();
                    m_isHolding = false;
                    m_holdTimer = 0.0f;
                }
            } else {
                // Key released, reset hold
                m_isHolding = false;
                m_holdTimer = 0.0f;
            }
        } else {
            // Instant interaction on key down (E key, Left Click, or Xbox X button)
            if (Input::GetKeyDown(interactKey) || Input::GetMouseButtonDown(0) || XboxController::GetButtonDown(XboxButton::X, 0)) {
                m_currentTarget->Interact();
            }
        }
    } else {
        // No target, reset hold state
        m_isHolding = false;
        m_holdTimer = 0.0f;
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

        // Try SpriteSheetRenderer (billboard)
        if (!isHit) {
            SpriteSheetRenderer* spriteSheetRenderer = obj->GetComponent<SpriteSheetRenderer>();
            if (spriteSheetRenderer && spriteSheetRenderer->billboard && spriteSheetRenderer->GetSpriteSheet()) {
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

                    // Use exact ray-intersection against the door's closed position.
                    // This limits interaction strictly within the door's tile horizontally,
                    // without needing a loose 45 degree angle check.
                    f32 x1, y1, x2, y2;
                    if (door->facing == DoorFacing::NorthSouth) {
                        // Line along X
                        x1 = static_cast<f32>(door->tileX);
                        y1 = static_cast<f32>(door->tileY) + 0.5f;
                        x2 = x1 + 1.0f;
                        y2 = y1;
                    } else {
                        // Line along Y
                        x1 = static_cast<f32>(door->tileX) + 0.5f;
                        y1 = static_cast<f32>(door->tileY);
                        x2 = x1;
                        y2 = y1 + 1.0f;
                    }

                    f32 tHit = -1.0f;
                    if (door->facing == DoorFacing::NorthSouth) {
                        if (fabsf(dirY) > 0.0001f) {
                            tHit = (y1 - camPos.y) / dirY;
                            if (tHit > 0) {
                                f32 hitX = camPos.x + tHit * dirX;
                                if (hitX < x1 || hitX > x2) tHit = -1.0f;
                            }
                        }
                    } else {
                        if (fabsf(dirX) > 0.0001f) {
                            tHit = (x1 - camPos.x) / dirX;
                            if (tHit > 0) {
                                f32 hitY = camPos.y + tHit * dirY;
                                if (hitY < y1 || hitY > y2) tHit = -1.0f;
                            }
                        }
                    }
                    
                    if (tHit > 0 && tHit <= interactable->maxDistance) {
                        isHit = true;
                        perpDistance = tHit; // tHit is exact Euclidean distance as dir is normalized
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
    
// Reset hold state when target changes
m_isHolding = false;
m_holdTimer = 0.0f;
    
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
