#pragma once
/**
 * @file Text.hpp
 * @brief UI Text component for rendering text
 */

#include "UIElement.hpp"
#include "../Graphics/Color.hpp"
#include <string>

namespace UI
{
    /**
     * @enum TextAnchor
     * @brief Text alignment within the rect
     */
    enum class TextAnchor
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
     * @class Text
     * @brief UI component for rendering text
     * 
     * Similar to Unity's Text component.
     */
    class Text : public UIElement
    {
    private:
        std::string m_text;
        s8 m_font = -1;                      // Font handle (-1 uses default)
        f32 m_fontSize = 1.0f;               // Scale factor
        Engine::Color m_color;                // Text color
        TextAnchor m_anchor = TextAnchor::MiddleCenter;

    public:
        Text();
        virtual ~Text() = default;

        /**
         * @brief Render the text
         */
        void Render() override;

        /**
         * @brief Set the text content
         */
        void SetText(const std::string& text) { m_text = text; }
        const std::string& GetText() const { return m_text; }

        /**
         * @brief Set the font handle (use -1 for default font)
         */
        void SetFont(s8 font) { m_font = font; }
        s8 GetFont() const { return m_font; }

        /**
         * @brief Set the font size (scale factor)
         */
        void SetFontSize(f32 size) { m_fontSize = size; }
        f32 GetFontSize() const { return m_fontSize; }

        /**
         * @brief Set the text color
         */
        void SetColor(const Engine::Color& color) { m_color = color; }
        const Engine::Color& GetColor() const { return m_color; }

        /**
         * @brief Set the text anchor/alignment
         */
        void SetAnchor(TextAnchor anchor) { m_anchor = anchor; }
        TextAnchor GetAnchor() const { return m_anchor; }

        /**
         * @brief Get the rendered text size in pixels
         * @param outWidth Output width
         * @param outHeight Output height
         */
        void GetTextSize(f32& outWidth, f32& outHeight) const;

    private:
        /**
         * @brief Get the font to use (default if m_font is -1)
         */
        s8 GetActiveFont() const;

        /**
         * @brief Calculate the normalized position for AEGfxPrint based on anchor
         */
        void CalculateTextPosition(f32& outX, f32& outY) const;
    };

} // namespace UI
