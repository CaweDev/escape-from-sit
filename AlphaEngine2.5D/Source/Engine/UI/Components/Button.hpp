#pragma once
/**
 * @file Button.hpp
 * @brief Interactive button UI component
 */

#include "Engine/UI/Core/UIElement.hpp"
#include "Engine/UI/Core/UIEventSystem.hpp"
#include "Engine/UI/Components/Image.hpp"
#include "Engine/UI/Components/Text.hpp"
#include "Engine/Graphics/Color.hpp"
#include "Engine/Graphics/Texture2D.hpp"
#include <functional>

namespace UI
{
    /**
     * @enum ButtonState
     * @brief Current state of the button
     */
    enum class ButtonState
    {
        Normal,
        Highlighted,
        Pressed,
        Disabled
    };

    /**
     * @struct ColorBlock
     * @brief Colors for each button state
     */
    struct ColorBlock
    {
        Engine::Color normalColor;
        Engine::Color highlightedColor;
        Engine::Color pressedColor;
        Engine::Color disabledColor;
        f32 fadeDuration = 0.1f;

        ColorBlock()
            : normalColor(Engine::Color::white())
            , highlightedColor(Engine::Color(0.9f, 0.9f, 0.9f, 1.0f))
            , pressedColor(Engine::Color(0.7f, 0.7f, 0.7f, 1.0f))
            , disabledColor(Engine::Color(0.5f, 0.5f, 0.5f, 0.5f))
        {
        }
    };

    /**
     * @class Button
     * @brief Interactive button with visual state transitions
     * 
     * Similar to Unity's Button component.
     */
    class Button : public UIElement,
                   public IPointerEnterHandler,
                   public IPointerExitHandler,
                   public IPointerDownHandler,
                   public IPointerUpHandler,
                   public IPointerClickHandler
    {
    private:
        Image* m_targetGraphic = nullptr;  // Owned background image
        Text* m_label = nullptr;           // Owned label text
        
        ButtonState m_state = ButtonState::Normal;
        ColorBlock m_colors;
        
        bool m_interactable = true;
        
        // Color transition
        Engine::Color m_currentColor;
        Engine::Color m_targetColor;
        f32 m_colorTransitionTime = 0;

    public:
        // Click callback
        std::function<void()> onClick;

        Button();
        virtual ~Button();

        /**
         * @brief Update button state and color transitions
         */
        void Update(f32 dt) override;

        /**
         * @brief Render the button
         */
        void Render() override;

        // Pointer event handlers
        void OnPointerEnter(const PointerEventData& eventData) override;
        void OnPointerExit(const PointerEventData& eventData) override;
        void OnPointerDown(const PointerEventData& eventData) override;
        void OnPointerUp(const PointerEventData& eventData) override;
        void OnPointerClick(const PointerEventData& eventData) override;

        /**
         * @brief Set whether the button is interactable
         */
        void SetInteractable(bool interactable);
        bool IsInteractable() const { return m_interactable; }

        /**
         * @brief Get the current button state
         */
        ButtonState GetState() const { return m_state; }

        /**
         * @brief Set the button colors
         */
        void SetColors(const ColorBlock& colors) { m_colors = colors; }
        const ColorBlock& GetColors() const { return m_colors; }

        /**
         * @brief Set the button text
         * @param text Text to display
         */
        void SetText(const std::string& text);
        std::string GetText() const;

        /**
         * @brief Set the background color (normal state)
         */
        void SetBackgroundColor(const Engine::Color& color);

        /**
         * @brief Set a background sprite/texture (enables 9-slice if texture has a border set)
         * @param texture Texture to use as background
         * @param sliced  If true, sets ImageType::Sliced for 9-slice rendering
         */
        void SetBackgroundSprite(Engine::Texture2D* texture, bool sliced = true);

        /**
         * @brief Get the background image
         */
        Image* GetTargetGraphic() const { return m_targetGraphic; }

        /**
         * @brief Get the label text component
         */
        Text* GetLabel() const { return m_label; }

    private:
        /**
         * @brief Update the target color based on state
         */
        void UpdateTargetColor();

        /**
         * @brief Set state and update target color
         */
        void SetState(ButtonState newState);
    };

} // namespace UI
