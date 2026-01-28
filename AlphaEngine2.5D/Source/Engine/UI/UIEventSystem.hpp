#pragma once
/**
 * @file UIEventSystem.hpp
 * @brief Event interfaces and data for UI pointer events
 */

#include "AEEngine.h"

namespace UI
{
    // Forward declaration
    class UIElement;

    /**
     * @struct PointerEventData
     * @brief Data passed to pointer event handlers
     */
    struct PointerEventData
    {
        f32 worldX = 0;         // World X position of pointer
        f32 worldY = 0;         // World Y position of pointer
        s32 screenX = 0;        // Screen X position
        s32 screenY = 0;        // Screen Y position
        bool isPressed = false; // Is the button currently pressed
        UIElement* target = nullptr; // Target element
    };

    /**
     * @class IPointerEnterHandler
     * @brief Interface for receiving pointer enter events
     */
    class IPointerEnterHandler
    {
    public:
        virtual ~IPointerEnterHandler() = default;
        virtual void OnPointerEnter(const PointerEventData& eventData) = 0;
    };

    /**
     * @class IPointerExitHandler
     * @brief Interface for receiving pointer exit events
     */
    class IPointerExitHandler
    {
    public:
        virtual ~IPointerExitHandler() = default;
        virtual void OnPointerExit(const PointerEventData& eventData) = 0;
    };

    /**
     * @class IPointerDownHandler
     * @brief Interface for receiving pointer down events
     */
    class IPointerDownHandler
    {
    public:
        virtual ~IPointerDownHandler() = default;
        virtual void OnPointerDown(const PointerEventData& eventData) = 0;
    };

    /**
     * @class IPointerUpHandler
     * @brief Interface for receiving pointer up events
     */
    class IPointerUpHandler
    {
    public:
        virtual ~IPointerUpHandler() = default;
        virtual void OnPointerUp(const PointerEventData& eventData) = 0;
    };

    /**
     * @class IPointerClickHandler
     * @brief Interface for receiving pointer click events
     */
    class IPointerClickHandler
    {
    public:
        virtual ~IPointerClickHandler() = default;
        virtual void OnPointerClick(const PointerEventData& eventData) = 0;
    };

} // namespace UI
