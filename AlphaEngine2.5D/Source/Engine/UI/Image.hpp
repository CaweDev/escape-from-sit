#pragma once
/**
 * @file Image.hpp
 * @brief UI Image component for rendering textures/sprites
 */

#include "UIElement.hpp"
#include "../Graphics/Texture2D.hpp"
#include "../Graphics/Color.hpp"

namespace UI
{
    /**
     * @enum ImageType
     * @brief How the image is rendered
     */
    enum class ImageType
    {
        Simple,     // Stretch to fill rect
        Sliced,     // 9-slice scaling (future enhancement)
    };

    /**
     * @class Image
     * @brief UI component for rendering textures/sprites
     * 
     * Similar to Unity's Image component. Can display:
     * - A texture sprite
     * - A solid color (when no texture is set)
     */
    class Image : public UIElement
    {
    private:
        Engine::Texture2D* m_texture = nullptr;      // Texture to render (not owned)
        Engine::Color m_color;               // Color tint/solid color
        ImageType m_imageType = ImageType::Simple;
        f32 m_fillAmount = 1.0f;            // For progress bars (0-1)

    public:
        Image();
        virtual ~Image() = default;

        /**
         * @brief Render the image
         */
        void Render() override;

        /**
         * @brief Set the texture
         * @param texture Texture to display (nullptr for solid color)
         */
        void SetTexture(Engine::Texture2D* texture) { m_texture = texture; }
        Engine::Texture2D* GetTexture() const { return m_texture; }

        /**
         * @brief Set the color (tint if texture, solid if no texture)
         */
        void SetColor(const Engine::Color& color) { m_color = color; }
        const Engine::Color& GetColor() const { return m_color; }

        /**
         * @brief Set the image type
         */
        void SetImageType(ImageType type) { m_imageType = type; }
        ImageType GetImageType() const { return m_imageType; }

        /**
         * @brief Set fill amount for progress bar style rendering
         * @param amount Fill percentage (0.0 - 1.0)
         */
        void SetFillAmount(f32 amount) { m_fillAmount = amount; }
        f32 GetFillAmount() const { return m_fillAmount; }
    };

} // namespace UI
