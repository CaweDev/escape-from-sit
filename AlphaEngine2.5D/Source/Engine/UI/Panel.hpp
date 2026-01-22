#pragma once
/**
 * @file Panel.hpp
 * @brief UI Panel component - container with optional background
 */

#include "UIElement.hpp"
#include "Image.hpp"
#include "../Graphics/Color.hpp"

namespace UI
{
    /**
     * @class Panel
     * @brief Container UI element with optional background
     * 
     * Similar to Unity's Panel. Used for grouping UI elements
     * and providing visual backgrounds.
     */
    class Panel : public UIElement
    {
    private:
        Image* m_background = nullptr;  // Owned background image

    public:
        Panel();
        virtual ~Panel();

        /**
         * @brief Update the panel
         */
        void Update(f32 dt) override;

        /**
         * @brief Render the panel (background + children)
         */
        void Render() override;

        /**
         * @brief Set a solid background color
         * @param color Background color
         */
        void SetBackgroundColor(const Engine::Color& color);

        /**
         * @brief Set a background sprite/texture
         * @param texture Background texture
         */
        void SetBackgroundSprite(Engine::Texture2D* texture);

        /**
         * @brief Get the background image (may be nullptr)
         */
        Image* GetBackground() const { return m_background; }

        /**
         * @brief Remove the background
         */
        void ClearBackground();

    private:
        /**
         * @brief Ensure background image exists
         */
        void EnsureBackground();
    };

} // namespace UI
