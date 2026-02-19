#pragma once
/**
 * @file VerticalLayoutGroup.hpp
 * @brief Vertical layout controller
 */

#include "Engine/UI/Layout/LayoutGroup.hpp"

namespace UI
{
    /**
     * @class VerticalLayoutGroup
     * @brief Arranges children in a vertical stack
     */
    class VerticalLayoutGroup : public LayoutGroup
    {
    private:
        bool m_childForceExpandWidth = false;
        bool m_childForceExpandHeight = false;

    public:
        VerticalLayoutGroup();
        virtual ~VerticalLayoutGroup() = default;

        /**
         * @brief Force children to expand to fill width
         */
        void SetChildForceExpandWidth(bool expand);
        bool GetChildForceExpandWidth() const { return m_childForceExpandWidth; }

        /**
         * @brief Force children to expand to fill available height
         */
        void SetChildForceExpandHeight(bool expand);
        bool GetChildForceExpandHeight() const { return m_childForceExpandHeight; }

    protected:
        /**
         * @brief Calculate vertical layout
         */
        void CalculateLayout() override;
    };

} // namespace UI
