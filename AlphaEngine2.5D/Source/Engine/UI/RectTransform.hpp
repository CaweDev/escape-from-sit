#pragma once
/**
 * @file RectTransform.hpp
 * @brief Unity-style RectTransform for UI element positioning and sizing
 */

#include "AEEngine.h"

namespace UI
{
    /**
     * @struct Rect
     * @brief Represents a rectangle in world coordinates
     */
    struct Rect
    {
        f32 x;      // Left edge x position
        f32 y;      // Bottom edge y position
        f32 width;  // Width of rectangle
        f32 height; // Height of rectangle

        Rect() : x(0), y(0), width(0), height(0) {}
        Rect(f32 x, f32 y, f32 w, f32 h) : x(x), y(y), width(w), height(h) {}

        f32 GetXMin() const { return x; }
        f32 GetXMax() const { return x + width; }
        f32 GetYMin() const { return y; }
        f32 GetYMax() const { return y + height; }
        f32 GetCenterX() const { return x + width * 0.5f; }
        f32 GetCenterY() const { return y + height * 0.5f; }

        bool Contains(f32 px, f32 py) const
        {
            return px >= x && px <= x + width && py >= y && py <= y + height;
        }
    };

    /**
     * @class RectTransform
     * @brief Defines position, size, and anchoring for UI elements (Unity-style)
     * 
     * Coordinate system:
     * - anchorMin/anchorMax: Normalized (0-1) relative to parent
     * - pivot: Normalized (0-1) relative to this element (0,0 = bottom-left, 1,1 = top-right)
     * - anchoredPosition: Offset in pixels from anchor midpoint
     * - sizeDelta: Size offset or fixed size
     */
    class RectTransform
    {
    public:
        // Anchor points relative to parent (0-1 normalized)
        AEVec2 anchorMin;       // Bottom-left anchor point
        AEVec2 anchorMax;       // Top-right anchor point
        
        // Pivot point for rotation/scaling (0-1 normalized, relative to this element)
        AEVec2 pivot;
        
        // Position offset from anchor midpoint in pixels
        AEVec2 anchoredPosition;
        
        // Size in pixels (when anchors are same point)
        // When anchors differ, this is the size delta from anchor-defined size
        AEVec2 sizeDelta;

        RectTransform()
            : anchorMin{ 0.5f, 0.5f }
            , anchorMax{ 0.5f, 0.5f }
            , pivot{ 0.5f, 0.5f }
            , anchoredPosition{ 0.0f, 0.0f }
            , sizeDelta{ 100.0f, 100.0f }
        {
        }

        /**
         * @brief Calculate the world-space rectangle given a parent rect
         * @param parentRect The parent element's world rect (or canvas rect for root elements)
         * @return World-space rectangle for this element
         */
        Rect CalculateWorldRect(const Rect& parentRect) const
        {
            // Calculate anchor positions in parent space
            f32 anchorLeft = parentRect.x + parentRect.width * anchorMin.x;
            f32 anchorRight = parentRect.x + parentRect.width * anchorMax.x;
            f32 anchorBottom = parentRect.y + parentRect.height * anchorMin.y;
            f32 anchorTop = parentRect.y + parentRect.height * anchorMax.y;

            f32 width, height, x, y;

            // Calculate width/height
            if (anchorMin.x == anchorMax.x)
            {
                width = sizeDelta.x;
            }
            else
            {
                width = (anchorRight - anchorLeft) + sizeDelta.x;
            }

            if (anchorMin.y == anchorMax.y)
            {
                height = sizeDelta.y;
            }
            else
            {
                height = (anchorTop - anchorBottom) + sizeDelta.y;
            }

            // The anchoredPosition is the offset from the anchor midpoint to the pivot
            f32 anchorCenterX = (anchorLeft + anchorRight) * 0.5f;
            f32 anchorCenterY = (anchorBottom + anchorTop) * 0.5f;

            x = anchorCenterX + anchoredPosition.x - width * pivot.x;
            y = anchorCenterY + anchoredPosition.y - height * pivot.y;

            return Rect(x, y, width, height);
        }

        /**
         * @brief Set anchors to stretch to fill parent
         */
        void SetStretchAll()
        {
            anchorMin = { 0.0f, 0.0f };
            anchorMax = { 1.0f, 1.0f };
            anchoredPosition = { 0.0f, 0.0f };
            sizeDelta = { 0.0f, 0.0f };
        }

        /**
         * @brief Set anchors to center with fixed size
         * @param width Width in pixels
         * @param height Height in pixels
         */
        void SetCentered(f32 width, f32 height)
        {
            anchorMin = { 0.5f, 0.5f };
            anchorMax = { 0.5f, 0.5f };
            pivot = { 0.5f, 0.5f };
            anchoredPosition = { 0.0f, 0.0f };
            sizeDelta = { width, height };
        }
    };

} // namespace UI
