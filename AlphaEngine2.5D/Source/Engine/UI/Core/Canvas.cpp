/**
 * @file Canvas.cpp
 * @brief Canvas implementation
 */

#include "Engine/UI/Core/Canvas.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/Math/Mathf.hpp"
#include "Engine/UI/Core/UIManager.hpp"
#include <algorithm>

namespace UI
{
    Canvas::~Canvas()
    {
        // Delete all root elements
        for (auto* element : m_rootElements)
        {
            delete element;
        }
        m_rootElements.clear();
    }

    void Canvas::SetReferenceResolution(f32 width, f32 height)
    {
        m_referenceWidth = width;
        m_referenceHeight = height;
        m_scaleFactorDirty = true;
    }

    void Canvas::GetReferenceResolution(f32& outWidth, f32& outHeight) const
    {
        outWidth = m_referenceWidth;
        outHeight = m_referenceHeight;
    }

    void Canvas::SetScaleMode(CanvasScaleMode mode)
    {
        m_scaleMode = mode;
        m_scaleFactorDirty = true;
    }

    void Canvas::SetSortOrder(s32 order)
    {
        if (m_sortOrder != order)
        {
            m_sortOrder = order;
            if (UIManager::GetInstance())
            {
                UIManager::GetInstance()->SetCanvasesSortDirty();
            }
        }
    }

    f32 Canvas::GetScaleFactor() const
    {
        if (m_scaleFactorDirty)
        {
            if (m_scaleMode == CanvasScaleMode::ConstantPixelSize)
            {
                m_scaleFactor = 1.0f;
            }
            else
            {
                f32 widthRatio = static_cast<f32>(Engine::Application::GetWindowWidth()) / m_referenceWidth;
                f32 heightRatio = static_cast<f32>(Engine::Application::GetWindowHeight()) / m_referenceHeight;
                
                // Use the smaller ratio to ensure UI fits
                m_scaleFactor = Engine::Mathf::Min(widthRatio, heightRatio);
            }
            m_scaleFactorDirty = false;
        }
        return m_scaleFactor;
    }

    Rect Canvas::GetCanvasRect() const
    {
        f32 sw = static_cast<f32>(Engine::Application::GetWindowWidth());
        f32 sh = static_cast<f32>(Engine::Application::GetWindowHeight());
 
        // Canvas rect in world coordinates centered at origin
        return Rect(-sw * 0.5f, -sh * 0.5f, sw, sh);
    }

    void Canvas::Update(f32 dt)
    {
        for (auto* element : m_rootElements)
        {
            if (element->enabled)
            {
                element->Update(dt);
            }
        }
    }

    void Canvas::Render()
    {
        // Ensure root elements are sorted by renderOrder
        SortElements();
 
        for (auto* element : m_rootElements)
        {
            if (element->visible)
            {
                element->Render();
            }
        }
    }

    void Canvas::AddElement(UIElement* element)
    {
        if (!element) return;

        // Check if already added
        auto it = std::find(m_rootElements.begin(), m_rootElements.end(), element);
        if (it != m_rootElements.end()) return;

        // Remove from previous parent if any
        if (element->GetParent())
        {
            element->GetParent()->RemoveChild(element);
        }

        element->SetCanvas(this);
        m_rootElements.push_back(element);
        
        element->SetDirty();
        m_sortDirty = true;
    }

    void Canvas::RemoveElement(UIElement* element)
    {
        if (!element) return;

        auto it = std::find(m_rootElements.begin(), m_rootElements.end(), element);
        if (it != m_rootElements.end())
        {
            (*it)->SetCanvas(nullptr);
            m_rootElements.erase(it);
        }
    }

    Engine::Vector2 Canvas::ScreenToCanvasPosition(s32 screenX, s32 screenY) const
    {
        f32 screenWidth = static_cast<f32>(Engine::Application::GetWindowWidth());
        f32 screenHeight = static_cast<f32>(Engine::Application::GetWindowHeight());
 
        // Screen coordinates: (0,0) at top-left, Y increases downward
        // World coordinates: (0,0) at center, Y increases upward
        return Engine::Vector2(static_cast<f32>(screenX) - screenWidth * 0.5f, screenHeight * 0.5f - static_cast<f32>(screenY));
    }

    UIElement* Canvas::Raycast(f32 worldX, f32 worldY) const
    {
        // Ensure elements are sorted first
        SortElements();

        // Raycast in reverse order (highest renderOrder first)
        for (auto it = m_rootElements.rbegin(); it != m_rootElements.rend(); ++it)
        {
            UIElement* hit = RaycastElement(*it, worldX, worldY);
            if (hit) return hit;
        }
        return nullptr;
    }

    UIElement* Canvas::RaycastElement(UIElement* element, f32 worldX, f32 worldY) const
    {
        if (!element || !element->enabled || !element->visible)
            return nullptr;
 
        // Ensure children are sorted before raycasting
        element->SortChildren();
 
        // Check children first (highest renderOrder first)
        const auto& children = element->GetChildren();
        for (auto it = children.rbegin(); it != children.rend(); ++it)
        {
            UIElement* hit = RaycastElement(*it, worldX, worldY);
            if (hit) return hit;
        }
 
        // Check this element
        if (element->ContainsPoint(worldX, worldY))
        {
            return element;
        }
 
        return nullptr;
    }
 
    void Canvas::SortElements() const
    {
        if (m_sortDirty && !m_rootElements.empty())
        {
            std::stable_sort(m_rootElements.begin(), m_rootElements.end(),
                [](const UIElement* a, const UIElement* b) {
                    return a->GetGlobalRenderOrder() < b->GetGlobalRenderOrder();
                });
            m_sortDirty = false;
        }
    }



} // namespace UI
