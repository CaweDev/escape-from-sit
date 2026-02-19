#pragma once
/**
 * @file UIElement.hpp
 * @brief Base class for all UI elements
 */

#include "AEEngine.h"
#include "RectTransform.hpp"
#include "../Graphics/Color.hpp"
#include <vector>
#include <string>

namespace UI
{
    // Forward declarations
    class Canvas;

    /**
     * @class UIElement
     * @brief Base class for all UI elements (Image, Text, Button, Panel, etc.)
     * 
     * Provides:
     * - RectTransform for positioning
     * - Parent/child hierarchy
     * - Enabled/visible state
     * - Virtual Update/Render methods
     */
    class UIElement
    {
    public:
        // Transform for positioning and sizing
        RectTransform rectTransform;

        // Element state
        bool enabled = true;    // If false, Update() is not called
        bool visible = true;    // If false, Render() is not called
        
        // If true, this element can receive pointer events
        bool raycastTarget = true;

        // Name for debugging
        std::string name;

    protected:
        s32 m_renderOrder = 0;
        UIElement* m_parent = nullptr;
        mutable std::vector<UIElement*> m_children;
        Canvas* m_canvas = nullptr;
        
        // Sorting and caching
        mutable bool m_childrenSortDirty = true;
        mutable Rect m_cachedWorldRect;
        mutable bool m_worldRectDirty = true;

    public:
        UIElement() = default;
        virtual ~UIElement();

        /**
         * @brief Get the render order for this element relative to its siblings
         * @param order Higher values are rendered later (on top)
         */
        void SetRenderOrder(s32 order);
        s32 GetRenderOrder() const { return m_renderOrder; }

        /**
         * @brief Get the global render order (cumulative from parents)
         */
        virtual s32 GetGlobalRenderOrder() const;

        /**
         * @brief Sort the children of this element by renderOrder
         */
        void SortChildren() const;

        // Prevent copying
        UIElement(const UIElement&) = delete;
        UIElement& operator=(const UIElement&) = delete;

        /**
         * @brief Update the element (called each frame if enabled)
         * @param dt Delta time in seconds
         */
        virtual void Update(f32 dt);

        /**
         * @brief Render the element (called each frame if visible)
         */
        virtual void Render();

        /**
         * @brief Get the world-space rectangle for this element
         * @return World rect calculated from parent hierarchy
         */
        Rect GetWorldRect() const;

        /**
         * @brief Check if a world-space point is inside this element
         * @param worldX X position in world coordinates
         * @param worldY Y position in world coordinates
         * @return True if point is inside
         */
        bool ContainsPoint(f32 worldX, f32 worldY) const;

        // Hierarchy management
        void SetParent(UIElement* parent);
        UIElement* GetParent() const { return m_parent; }
        
        void AddChild(UIElement* child);
        void RemoveChild(UIElement* child);
        const std::vector<UIElement*>& GetChildren() const { return m_children; }

        // Canvas access
        void SetCanvas(Canvas* canvas);
        Canvas* GetCanvas() const { return m_canvas; }

        /**
         * @brief Set the anchored position (offset from anchor midpoint)
         */
        void SetAnchoredPosition(const AEVec2& pos);
        AEVec2 GetAnchoredPosition() const { return rectTransform.anchoredPosition; }

        /**
         * @brief Set the size delta (size relative to anchors)
         */
        void SetSizeDelta(const AEVec2& size);
        AEVec2 GetSizeDelta() const { return rectTransform.sizeDelta; }

        /**
         * @brief Set the pivot point (0-1 normalized)
         */
        void SetPivot(const AEVec2& pivot);
        AEVec2 GetPivot() const { return rectTransform.pivot; }

        /**
         * @brief Set the anchor points (0-1 normalized relative to parent)
         */
        void SetAnchors(const AEVec2& min, const AEVec2& max);
        void SetAnchorMin(const AEVec2& min);
        void SetAnchorMax(const AEVec2& max);
        AEVec2 GetAnchorMin() const { return rectTransform.anchorMin; }
        AEVec2 GetAnchorMax() const { return rectTransform.anchorMax; }

        /**
         * @brief Mark this element's world rect as needing recalculation
         */
        void SetDirty();

        /**
         * @brief Create a child element of the specified type
         * @tparam T Type of element to create (must inherit from UIElement)
         * @return Pointer to the created element
         */
        template<typename T>
        T* CreateChild()
        {
            static_assert(std::is_base_of<UIElement, T>::value, "T must inherit from UIElement");
            T* child = new T();
            AddChild(child);
            return child;
        }

    protected:
        /**
         * @brief Get the parent's world rect (or canvas rect for root elements)
         */
        Rect GetParentRect() const;
    };

} // namespace UI
