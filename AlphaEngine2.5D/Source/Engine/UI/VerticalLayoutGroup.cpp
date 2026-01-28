/**
 * @file VerticalLayoutGroup.cpp
 * @brief Vertical layout implementation
 */

#include "VerticalLayoutGroup.hpp"

namespace UI
{
    VerticalLayoutGroup::VerticalLayoutGroup()
    {
        name = "VerticalLayoutGroup";
    }

    void VerticalLayoutGroup::SetChildForceExpandWidth(bool expand)
    {
        m_childForceExpandWidth = expand;
        m_layoutDirty = true;
    }

    void VerticalLayoutGroup::SetChildForceExpandHeight(bool expand)
    {
        m_childForceExpandHeight = expand;
        m_layoutDirty = true;
    }

    void VerticalLayoutGroup::CalculateLayout()
    {
        const auto& children = GetChildren();
        if (children.empty()) return;

        Rect contentRect = GetContentRect();

        // Calculate total height needed
        f32 totalHeight = 0;
        s32 childCount = static_cast<s32>(children.size());
        
        for (auto* child : children)
        {
            if (!child->enabled) continue;
            totalHeight += child->rectTransform.sizeDelta.y;
        }
        totalHeight += m_spacing * (childCount - 1);

        // Calculate starting Y position based on alignment
        f32 startY = contentRect.GetYMax();
        
        switch (m_childAlignment)
        {
        case ChildAlignment::UpperLeft:
        case ChildAlignment::UpperCenter:
        case ChildAlignment::UpperRight:
            startY = contentRect.GetYMax();
            break;
        case ChildAlignment::MiddleLeft:
        case ChildAlignment::MiddleCenter:
        case ChildAlignment::MiddleRight:
            startY = contentRect.GetCenterY() + totalHeight * 0.5f;
            break;
        case ChildAlignment::LowerLeft:
        case ChildAlignment::LowerCenter:
        case ChildAlignment::LowerRight:
            startY = contentRect.GetYMin() + totalHeight;
            break;
        }

        // Position children from top to bottom
        f32 currentY = startY;
        
        for (auto* child : children)
        {
            if (!child->enabled) continue;

            f32 childWidth = m_childForceExpandWidth ? contentRect.width : child->rectTransform.sizeDelta.x;
            f32 childHeight = child->rectTransform.sizeDelta.y;

            // Calculate X position based on alignment
            f32 childX = contentRect.x;
            switch (m_childAlignment)
            {
            case ChildAlignment::UpperLeft:
            case ChildAlignment::MiddleLeft:
            case ChildAlignment::LowerLeft:
                childX = contentRect.GetXMin() + childWidth * 0.5f;
                break;
            case ChildAlignment::UpperCenter:
            case ChildAlignment::MiddleCenter:
            case ChildAlignment::LowerCenter:
                childX = contentRect.GetCenterX();
                break;
            case ChildAlignment::UpperRight:
            case ChildAlignment::MiddleRight:
            case ChildAlignment::LowerRight:
                childX = contentRect.GetXMax() - childWidth * 0.5f;
                break;
            }

            // Set child position (top of child at currentY)
            currentY -= childHeight * 0.5f;
            child->rectTransform.anchorMin = { 0.5f, 0.5f };
            child->rectTransform.anchorMax = { 0.5f, 0.5f };
            child->rectTransform.pivot = { 0.5f, 0.5f };
            child->rectTransform.anchoredPosition = { childX - contentRect.GetCenterX(), currentY - contentRect.GetCenterY() };
            child->rectTransform.sizeDelta = { childWidth, childHeight };
            
            currentY -= childHeight * 0.5f + m_spacing;
            child->SetDirty();
        }
    }

} // namespace UI
