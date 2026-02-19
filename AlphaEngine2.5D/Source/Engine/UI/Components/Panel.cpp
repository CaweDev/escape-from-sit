/**
 * @file Panel.cpp
 * @brief Panel component implementation
 */

#include "Engine/UI/Components/Panel.hpp"

namespace UI
{
    Panel::Panel()
    {
        name = "Panel";
    }

    Panel::~Panel()
    {
        // Background will be deleted by UIElement destructor if it's a child.
        // We just need to clear the pointer.
        m_background = nullptr;
    }

    void Panel::Update(f32 dt)
    {
        UIElement::Update(dt);
    }

    void Panel::Render()
    {
        UIElement::Render();
    }

    void Panel::SetBackgroundColor(const Engine::Color& color)
    {
        EnsureBackground();
        m_background->SetTexture(nullptr);
        m_background->SetColor(color);
    }

    void Panel::SetBackgroundSprite(Engine::Texture2D* texture)
    {
        EnsureBackground();
        m_background->SetTexture(texture);
        m_background->SetColor(Engine::Color::white());
    }

    void Panel::ClearBackground()
    {
        if (m_background)
        {
            RemoveChild(m_background);
            delete m_background;
            m_background = nullptr;
        }
    }

    void Panel::EnsureBackground()
    {
        if (!m_background)
        {
            m_background = new Image();
            m_background->name = "Panel Background";
            m_background->raycastTarget = false;
            
            // Add as child to handle lifecycle and rendering
            AddChild(m_background);
            
            // Set to stretch and fill the panel
            m_background->rectTransform.SetStretchAll();
        }
    }

} // namespace UI
