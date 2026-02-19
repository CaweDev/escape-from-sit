/**
 * @file HorizontalLayoutGroup.cpp
 * @brief Horizontal layout implementation
 */

#include "HorizontalLayoutGroup.hpp"

namespace UI
{
    HorizontalLayoutGroup::HorizontalLayoutGroup()
    {
        name = "HorizontalLayoutGroup";
    }

    void HorizontalLayoutGroup::SetChildForceExpandWidth(bool expand)
    {
        m_childForceExpandWidth = expand;
        m_layoutDirty = true;
    }

    void HorizontalLayoutGroup::SetChildForceExpandHeight(bool expand)
    {
        m_childForceExpandHeight = expand;
        m_layoutDirty = true;
    }

    void HorizontalLayoutGroup::CalculateLayout()
    {
        const auto& children = GetChildren();
        if (children.empty()) return;

        Rect contentRect = GetContentRect();

        // Calculate total width needed
        f32 totalWidth = 0;
        s32 childCount = static_cast<s32>(children.size());
        
        for (auto* child : children)
        {
            if (!child->enabled) continue;
            totalWidth += child->GetSizeDelta().x;
        }
        totalWidth += m_spacing * (childCount - 1);

        // Calculate starting X position based on alignment
        f32 startX = contentRect.GetXMin();
        
        switch (m_childAlignment)
        {
        case ChildAlignment::UpperLeft:
        case ChildAlignment::MiddleLeft:
        case ChildAlignment::LowerLeft:
            startX = contentRect.GetXMin();
            break;
        case ChildAlignment::UpperCenter:
        case ChildAlignment::MiddleCenter:
        case ChildAlignment::LowerCenter:
            startX = contentRect.GetCenterX() - totalWidth * 0.5f;
            break;
        case ChildAlignment::UpperRight:
        case ChildAlignment::MiddleRight:
        case ChildAlignment::LowerRight:
            startX = contentRect.GetXMax() - totalWidth;
            break;
        }

        // Position children from left to right
        f32 currentX = startX;
        
        for (auto* child : children)
        {
            if (!child->enabled) continue;

            f32 childWidth = child->GetSizeDelta().x;
            f32 childHeight = m_childForceExpandHeight ? contentRect.height : child->GetSizeDelta().y;

            // Calculate Y position based on alignment
            f32 childY = contentRect.GetCenterY();
            switch (m_childAlignment)
            {
            case ChildAlignment::UpperLeft:
            case ChildAlignment::UpperCenter:
            case ChildAlignment::UpperRight:
                childY = contentRect.GetYMax() - childHeight * 0.5f;
                break;
            case ChildAlignment::MiddleLeft:
            case ChildAlignment::MiddleCenter:
            case ChildAlignment::MiddleRight:
                childY = contentRect.GetCenterY();
                break;
            case ChildAlignment::LowerLeft:
            case ChildAlignment::LowerCenter:
            case ChildAlignment::LowerRight:
                childY = contentRect.GetYMin() + childHeight * 0.5f;
                break;
            }

            // Set child position (left of child at currentX)
            currentX += childWidth * 0.5f;
            child->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
            child->SetPivot({ 0.5f, 0.5f });
            child->SetAnchoredPosition({ currentX - contentRect.GetCenterX(), childY - contentRect.GetCenterY() });
            child->SetSizeDelta({ childWidth, childHeight });
            
            currentX += childWidth * 0.5f + m_spacing;
            child->SetDirty();
        }
    }

} // namespace UI
