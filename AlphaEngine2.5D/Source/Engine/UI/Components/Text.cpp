/**
 * @file Text.cpp
 * @brief Text component implementation
 */

#include "Engine/UI/Components/Text.hpp"
#include "Engine/UI/Core/UIManager.hpp"
#include "Engine/Core/Application.hpp"

namespace UI
{
    Text::Text()
        : m_color(Engine::Color::white())
    {
        name = "Text";
    }

    void Text::Render()
    {
        if (!visible || m_text.empty()) return;

        s8 font = GetActiveFont();
        if (font < 0) return;

        f32 x, y;
        CalculateTextPosition(x, y);

        AEGfxPrint(font, m_text.c_str(), x, y, m_fontSize,
                   m_color.r, m_color.g, m_color.b, m_color.a);

        // Render children
        UIElement::Render();
    }

    void Text::GetTextSize(f32& outWidth, f32& outHeight) const
    {
        s8 font = GetActiveFont();
        if (font < 0 || m_text.empty())
        {
            outWidth = 0;
            outHeight = 0;
            return;
        }

        AEGfxGetPrintSize(font, m_text.c_str(), m_fontSize, &outWidth, &outHeight);
    }

    s8 Text::GetActiveFont() const
    {
        if (m_font >= 0) return m_font;
        
        UIManager* mgr = UIManager::GetInstance();
        return mgr ? mgr->GetDefaultFont() : -1;
    }

    void Text::CalculateTextPosition(f32& outX, f32& outY) const
    {
        Rect worldRect = GetWorldRect();
        
        // Get text size in normalized coordinates
        f32 textWidth, textHeight;
        GetTextSize(textWidth, textHeight);

        // Get screen dimensions for conversion
        f32 sw = static_cast<f32>(Engine::Application::GetWindowWidth());
        f32 sh = static_cast<f32>(Engine::Application::GetWindowHeight());
 
        // Convert world rect to normalized coordinates (-1 to 1)
        f32 normLeft = worldRect.x / (sw * 0.5f);
        f32 normRight = (worldRect.x + worldRect.width) / (sw * 0.5f);
        f32 normBottom = worldRect.y / (sh * 0.5f);
        f32 normTop = (worldRect.y + worldRect.height) / (sh * 0.5f);
        

        // Calculate position based on anchor
        // AEGfxPrint draws from bottom-left of text
        switch (m_anchor)
        {
        case TextAnchor::UpperLeft:
            outX = normLeft;
            outY = normTop - textHeight;
            break;
        case TextAnchor::UpperCenter:
            outX = (normLeft + normRight) * 0.5f - textWidth * 0.5f;
            outY = normTop - textHeight;
            break;
        case TextAnchor::UpperRight:
            outX = normRight - textWidth;
            outY = normTop - textHeight;
            break;
        case TextAnchor::MiddleLeft:
            outX = normLeft;
            outY = (normBottom + normTop) * 0.5f - textHeight * 0.5f;
            break;
        case TextAnchor::MiddleCenter:
            outX = (normLeft + normRight) * 0.5f - textWidth * 0.5f;
            outY = (normBottom + normTop) * 0.5f - textHeight * 0.5f;
            break;
        case TextAnchor::MiddleRight:
            outX = normRight - textWidth;
            outY = (normBottom + normTop) * 0.5f - textHeight * 0.5f;
            break;
        case TextAnchor::LowerLeft:
            outX = normLeft;
            outY = normBottom;
            break;
        case TextAnchor::LowerCenter:
            outX = (normLeft + normRight) * 0.5f - textWidth * 0.5f;
            outY = normBottom;
            break;
        case TextAnchor::LowerRight:
            outX = normRight - textWidth;
            outY = normBottom;
            break;
        }
    }

} // namespace UI
