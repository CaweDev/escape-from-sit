/**
 * @file Button.cpp
 * @brief Button component implementation
 */

#include "Button.hpp"
#include "UIManager.hpp"

namespace UI
{
    Button::Button()
        : m_currentColor(Engine::Color::white())
        , m_targetColor(Engine::Color::white())
    {
        name = "Button";
 
        // Create background image
        m_targetGraphic = new Image();
        m_targetGraphic->name = "Button Background";
        m_targetGraphic->SetColor(m_colors.normalColor);
        m_targetGraphic->raycastTarget = false;
        AddChild(m_targetGraphic);
        m_targetGraphic->rectTransform.SetStretchAll();
 
        // Create label text
        m_label = new Text();
        m_label->name = "Button Label";
        m_label->SetAnchor(TextAnchor::MiddleCenter);
        m_label->SetColor(Engine::Color::black());
        m_label->raycastTarget = false;
        AddChild(m_label);
        m_label->rectTransform.SetStretchAll();
 
        UpdateTargetColor();
        m_currentColor = m_targetColor;
    }

    Button::~Button()
    {
        // Sub-elements are deleted by UIElement destructor
        m_targetGraphic = nullptr;
        m_label = nullptr;
    }

    void Button::Update(f32 dt)
    {
        if (!enabled) return;

        // Update color transition
        if (m_colorTransitionTime < m_colors.fadeDuration)
        {
            m_colorTransitionTime += dt;
            f32 t = m_colorTransitionTime / m_colors.fadeDuration;
            if (t > 1.0f) t = 1.0f;

            m_currentColor = Engine::Color::Lerp(m_currentColor, m_targetColor, t);
            
            if (m_targetGraphic)
            {
                m_targetGraphic->SetColor(m_currentColor);
            }
        }

        // Update children
        UIElement::Update(dt);
    }

    void Button::Render()
    {
        UIElement::Render();
    }

    void Button::OnPointerEnter(const PointerEventData& eventData)
    {
        (void)eventData;
        if (!m_interactable) return;

        if (m_state == ButtonState::Normal)
        {
            SetState(ButtonState::Highlighted);
        }
    }

    void Button::OnPointerExit(const PointerEventData& eventData)
    {
        (void)eventData;
        if (!m_interactable) return;

        if (m_state == ButtonState::Highlighted || m_state == ButtonState::Pressed)
        {
            SetState(ButtonState::Normal);
        }
    }

    void Button::OnPointerDown(const PointerEventData& eventData)
    {
        (void)eventData;
        if (!m_interactable) return;

        SetState(ButtonState::Pressed);
    }

    void Button::OnPointerUp(const PointerEventData& eventData)
    {
        (void)eventData;
        if (!m_interactable) return;

        if (m_state == ButtonState::Pressed)
        {
            SetState(ButtonState::Highlighted);
        }
    }

    void Button::OnPointerClick(const PointerEventData& eventData)
    {
        (void)eventData;
        if (!m_interactable) return;

        if (onClick)
        {
            onClick();
        }
    }

    void Button::SetInteractable(bool interactable)
    {
        m_interactable = interactable;
        
        if (!interactable)
        {
            SetState(ButtonState::Disabled);
        }
        else if (m_state == ButtonState::Disabled)
        {
            SetState(ButtonState::Normal);
        }
    }

    void Button::SetText(const std::string& text)
    {
        if (m_label)
        {
            m_label->SetText(text);
        }
    }

    std::string Button::GetText() const
    {
        return m_label ? m_label->GetText() : "";
    }

    void Button::SetBackgroundColor(const Engine::Color& color)
    {
        m_colors.normalColor = color;
        // Derive other colors from normal color
        m_colors.highlightedColor = Engine::Color(
            color.r * 1.1f, color.g * 1.1f, color.b * 1.1f, color.a);
        m_colors.pressedColor = Engine::Color(
            color.r * 0.8f, color.g * 0.8f, color.b * 0.8f, color.a);
        m_colors.disabledColor = Engine::Color(
            color.r * 0.5f, color.g * 0.5f, color.b * 0.5f, color.a * 0.5f);
        
        UpdateTargetColor();
    }

    void Button::UpdateTargetColor()
    {
        switch (m_state)
        {
        case ButtonState::Normal:
            m_targetColor = m_colors.normalColor;
            break;
        case ButtonState::Highlighted:
            m_targetColor = m_colors.highlightedColor;
            break;
        case ButtonState::Pressed:
            m_targetColor = m_colors.pressedColor;
            break;
        case ButtonState::Disabled:
            m_targetColor = m_colors.disabledColor;
            break;
        }
    }

    void Button::SetState(ButtonState newState)
    {
        if (m_state != newState)
        {
            m_state = newState;
            m_colorTransitionTime = 0;
            UpdateTargetColor();
        }
    }

} // namespace UI
