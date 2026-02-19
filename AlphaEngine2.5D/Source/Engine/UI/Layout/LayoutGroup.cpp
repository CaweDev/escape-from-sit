/**
 * @file LayoutGroup.cpp
 * @brief LayoutGroup base class implementation
 */

#include "Engine/UI/Layout/LayoutGroup.hpp"

namespace UI
{
    LayoutGroup::LayoutGroup()
    {
        name = "LayoutGroup";
    }

    void LayoutGroup::Update(f32 dt)
    {
        if (!enabled) return;

        // Recalculate layout if dirty
        if (m_layoutDirty)
        {
            CalculateLayout();
            m_layoutDirty = false;
        }

        // Update children
        UIElement::Update(dt);
    }

    void LayoutGroup::SetPadding(const Padding& padding)
    {
        m_padding = padding;
        m_layoutDirty = true;
    }

    void LayoutGroup::SetSpacing(f32 spacing)
    {
        m_spacing = spacing;
        m_layoutDirty = true;
    }

    void LayoutGroup::SetChildAlignment(ChildAlignment alignment)
    {
        m_childAlignment = alignment;
        m_layoutDirty = true;
    }

    void LayoutGroup::AddChild(UIElement* child)
    {
        UIElement::AddChild(child);
        m_layoutDirty = true;
    }

    void LayoutGroup::RemoveChild(UIElement* child)
    {
        UIElement::RemoveChild(child);
        m_layoutDirty = true;
    }

    Rect LayoutGroup::GetContentRect() const
    {
        Rect worldRect = GetWorldRect();
        return Rect(
            worldRect.x + m_padding.left,
            worldRect.y + m_padding.bottom,
            worldRect.width - m_padding.left - m_padding.right,
            worldRect.height - m_padding.top - m_padding.bottom
        );
    }

} // namespace UI
