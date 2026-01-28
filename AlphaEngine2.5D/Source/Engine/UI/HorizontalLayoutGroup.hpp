#pragma once
/**
 * @file HorizontalLayoutGroup.hpp
 * @brief Horizontal layout controller
 */

#include "LayoutGroup.hpp"

namespace UI
{
    /**
     * @class HorizontalLayoutGroup
     * @brief Arranges children in a horizontal row
     */
    class HorizontalLayoutGroup : public LayoutGroup
    {
    private:
        bool m_childForceExpandWidth = false;
        bool m_childForceExpandHeight = false;

    public:
        HorizontalLayoutGroup();
        virtual ~HorizontalLayoutGroup() = default;

        /**
         * @brief Force children to expand to fill available width
         */
        void SetChildForceExpandWidth(bool expand);
        bool GetChildForceExpandWidth() const { return m_childForceExpandWidth; }

        /**
         * @brief Force children to expand to fill height
         */
        void SetChildForceExpandHeight(bool expand);
        bool GetChildForceExpandHeight() const { return m_childForceExpandHeight; }

    protected:
        /**
         * @brief Calculate horizontal layout
         */
        void CalculateLayout() override;
    };

} // namespace UI
