/**
 * @file UIElement.cpp
 * @brief UIElement base class implementation
 */

#include "UIElement.hpp"
#include "Canvas.hpp"
#include "UIManager.hpp"
#include "../Core/Application.hpp"
#include <algorithm>

namespace UI
{
    UIElement::~UIElement()
    {
        // Notify UIManager for safety
        UIManager* mgr = UIManager::GetInstance();
        if (mgr)
        {
            mgr->OnElementDestroyed(this);
        }

        // Remove from parent
        if (m_parent)
        {
            m_parent->RemoveChild(this);
        }

        // Delete all children
        for (auto* child : m_children)
        {
            child->m_parent = nullptr; // Prevent child from trying to remove itself
            delete child;
        }
        m_children.clear();
    }

    void UIElement::Update(f32 dt)
    {
        if (!enabled) return;

        // Update children
        for (auto* child : m_children)
        {
            child->Update(dt);
        }
    }

    void UIElement::Render()
    {
        if (!visible) return;
 
        // Ensure children are sorted by renderOrder
        SortChildren();
 
        // Render children
        for (auto* child : m_children)
        {
            child->Render();
        }
    }

    Rect UIElement::GetWorldRect() const
    {
        if (m_worldRectDirty)
        {
            Rect parentRect = GetParentRect();
            m_cachedWorldRect = rectTransform.CalculateWorldRect(parentRect);
            m_worldRectDirty = false;
        }
        return m_cachedWorldRect;
    }

    bool UIElement::ContainsPoint(f32 worldX, f32 worldY) const
    {
        if (!raycastTarget) return false;
        Rect rect = GetWorldRect();
        return rect.Contains(worldX, worldY);
    }

    void UIElement::SetParent(UIElement* parent)
    {
        if (m_parent == parent) return;

        // Remove from old parent
        if (m_parent)
        {
            m_parent->RemoveChild(this);
        }

        m_parent = parent;

        // Add to new parent
        if (parent)
        {
            parent->m_children.push_back(this);
            m_canvas = parent->m_canvas;
        }
        else
        {
            m_canvas = nullptr;
        }

        SetDirty();
    }

    void UIElement::AddChild(UIElement* child)
    {
        if (!child || child->m_parent == this) return;

        // Remove from old parent
        if (child->m_parent)
        {
            child->m_parent->RemoveChild(child);
        }

        child->m_parent = this;
        child->m_canvas = m_canvas;
        m_children.push_back(child);
        m_childrenSortDirty = true;
        
        child->SetDirty();
    }

    void UIElement::RemoveChild(UIElement* child)
    {
        if (!child) return;

        auto it = std::find(m_children.begin(), m_children.end(), child);
        if (it != m_children.end())
        {
            (*it)->m_parent = nullptr;
            (*it)->m_canvas = nullptr;
            m_children.erase(it);
        }
    }

    void UIElement::SetCanvas(Canvas* canvas)
    {
        m_canvas = canvas;
        if (m_canvas && !m_parent)
        {
            m_canvas->SetSortDirty();
        }
        for (auto* child : m_children)
        {
            child->SetCanvas(canvas);
        }
        SetDirty();
    }

    void UIElement::SetDirty()
    {
        m_worldRectDirty = true;
        for (auto* child : m_children)
        {
            child->SetDirty();
        }
    }

    Rect UIElement::GetParentRect() const
    {
        if (m_parent)
        {
            return m_parent->GetWorldRect();
        }
        else if (m_canvas)
        {
            return m_canvas->GetCanvasRect();
        }
        else
        {
            // Fallback to screen size
            f32 screenWidth = static_cast<f32>(Engine::Application::GetWindowWidth());
            f32 screenHeight = static_cast<f32>(Engine::Application::GetWindowHeight());
            return Rect(
                -screenWidth * 0.5f,
                -screenHeight * 0.5f,
                screenWidth,
                screenHeight
            );
        }
    }

    void UIElement::SetRenderOrder(s32 order)
    {
        if (m_renderOrder != order)
        {
            m_renderOrder = order;
            if (m_parent)
            {
                m_parent->m_childrenSortDirty = true;
            }
            else if (m_canvas)
            {
                m_canvas->SetSortDirty();
            }
        }
    }

    void UIElement::SortChildren() const
    {
        if (m_childrenSortDirty && !m_children.empty())
        {
            std::stable_sort(m_children.begin(), m_children.end(),
                [](const UIElement* a, const UIElement* b) {
                    return a->GetGlobalRenderOrder() < b->GetGlobalRenderOrder();
                });
            m_childrenSortDirty = false;
        }
    }

    s32 UIElement::GetGlobalRenderOrder() const
    {
        return m_renderOrder + (m_parent ? m_parent->GetGlobalRenderOrder() : 0);
    }

    void UIElement::SetAnchoredPosition(const AEVec2& pos)
    {
        rectTransform.anchoredPosition = pos;
        SetDirty();
    }

    void UIElement::SetSizeDelta(const AEVec2& size)
    {
        rectTransform.sizeDelta = size;
        SetDirty();
    }

    void UIElement::SetPivot(const AEVec2& pivot)
    {
        rectTransform.pivot = pivot;
        SetDirty();
    }

    void UIElement::SetAnchors(const AEVec2& min, const AEVec2& max)
    {
        rectTransform.anchorMin = min;
        rectTransform.anchorMax = max;
        SetDirty();
    }

} // namespace UI
