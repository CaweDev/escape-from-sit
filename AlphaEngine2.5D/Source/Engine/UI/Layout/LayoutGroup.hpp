#pragma once
/**
 * @file LayoutGroup.hpp
 * @brief Base class for layout controllers
 */

#include "Engine/UI/Core/UIElement.hpp"

namespace UI
{
    /**
     * @enum ChildAlignment
     * @brief How children are aligned within the layout
     */
    enum class ChildAlignment
    {
        UpperLeft,
        UpperCenter,
        UpperRight,
        MiddleLeft,
        MiddleCenter,
        MiddleRight,
        LowerLeft,
        LowerCenter,
        LowerRight
    };

    /**
     * @struct Padding
     * @brief Padding around layout content
     */
    struct Padding
    {
        f32 left = 0;
        f32 right = 0;
        f32 top = 0;
        f32 bottom = 0;

        Padding() = default;
        Padding(f32 all) : left(all), right(all), top(all), bottom(all) {}
        Padding(f32 horizontal, f32 vertical) 
            : left(horizontal), right(horizontal), top(vertical), bottom(vertical) {}
        Padding(f32 l, f32 r, f32 t, f32 b) : left(l), right(r), top(t), bottom(b) {}
    };

    /**
     * @class LayoutGroup
     * @brief Base class for automatic layout controllers
     * 
     * Subclasses implement specific layout algorithms
     * (vertical, horizontal, grid, etc.)
     */
    class LayoutGroup : public UIElement
    {
    protected:
        Padding m_padding;
        f32 m_spacing = 0;
        ChildAlignment m_childAlignment = ChildAlignment::UpperLeft;
        bool m_layoutDirty = true;

    public:
        LayoutGroup();
        virtual ~LayoutGroup() = default;

        /**
         * @brief Update - recalculates layout if dirty
         */
        void Update(f32 dt) override;

        /**
         * @brief Set padding around content
         */
        void SetPadding(const Padding& padding);
        const Padding& GetPadding() const { return m_padding; }

        /**
         * @brief Set spacing between children
         */
        void SetSpacing(f32 spacing);
        f32 GetSpacing() const { return m_spacing; }

        /**
         * @brief Set child alignment
         */
        void SetChildAlignment(ChildAlignment alignment);
        ChildAlignment GetChildAlignment() const { return m_childAlignment; }

        /**
         * @brief Mark layout as needing recalculation
         */
        void SetLayoutDirty() { m_layoutDirty = true; }

        /**
         * @brief Add child and mark layout dirty
         */
        void AddChild(UIElement* child);

        /**
         * @brief Remove child and mark layout dirty
         */
        void RemoveChild(UIElement* child);

    protected:
        /**
         * @brief Calculate and apply layout to children (override in subclasses)
         */
        virtual void CalculateLayout() = 0;

        /**
         * @brief Get the content area (parent rect minus padding)
         */
        Rect GetContentRect() const;
    };

} // namespace UI
