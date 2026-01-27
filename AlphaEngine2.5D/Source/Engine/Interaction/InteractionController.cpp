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
#include <cfloat>

namespace Engine {

void InteractionController::Initialize() {
    // Create UI canvas for interaction elements
    m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
    
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
    
    int sw = Application::GetWindowWidth();
    int sh = Application::GetWindowHeight();
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
        
        // Check if it has a billboard SpriteRenderer
        SpriteRenderer* renderer = obj->GetComponent<SpriteRenderer>();
        if (!renderer || !renderer->billboard || !renderer->sprite) continue;
        
        // Project to screen
        ProjectionResult proj = ProjectionHelper::ProjectToScreen(
            obj->transform->position,
            Camera::main->transform->position,
            Camera::main->transform->rotation,
            Camera::main->fov,
            sw, sh
        );
        
        if (!proj.isVisible) continue;
        if (proj.perpDistance > interactable->maxDistance) continue;
        
        // Calculate screen bounds of the sprite
        f32 aspectRatio = static_cast<f32>(renderer->sprite->width) / 
                         static_cast<f32>(renderer->sprite->height);
        f32 spriteHeight = proj.scale * obj->transform->localScale.y;
        f32 spriteWidth = spriteHeight * aspectRatio * obj->transform->localScale.x / obj->transform->localScale.y;
        
        // Screen bounds (screenY is at the sprite's bottom)
        f32 left = proj.screenX - spriteWidth / 2.0f;
        f32 right = proj.screenX + spriteWidth / 2.0f;
        f32 bottom = proj.screenY;
        f32 top = proj.screenY - spriteHeight;
        
        // Check if screen center is inside sprite bounds
        if (screenCenterX >= left && screenCenterX <= right &&
            screenCenterY >= top && screenCenterY <= bottom) {
            
            // Pick the closest one
            if (proj.perpDistance < closestDist) {
                closestDist = proj.perpDistance;
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
        // Set new target renderer
        m_currentTargetRenderer = target->gameObject->GetComponent<SpriteRenderer>();
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
