#pragma once
/**
 * @file Canvas.hpp
 * @brief Root container for UI elements
 */

#include "AEEngine.h"
#include "Engine/UI/Core/RectTransform.hpp"
#include "Engine/UI/Core/UIElement.hpp"
#include <vector>

namespace UI
{
    /**
     * @enum CanvasScaleMode
     * @brief How the canvas scales with screen size
     */
    enum class CanvasScaleMode
    {
        ConstantPixelSize,  // UI stays same pixel size regardless of resolution
        ScaleWithScreenSize // UI scales to match reference resolution
    };

    /**
     * @class Canvas
     * @brief Root container for UI elements, similar to Unity's Canvas
     * 
     * All UI elements must be children of a Canvas to be rendered.
     * Handles resolution scaling and provides the root coordinate system.
     */
    class Canvas
    {
    private:
        mutable std::vector<UIElement*> m_rootElements;
        
        // Reference resolution for scaling
        f32 m_referenceWidth = 1280.f;
        f32 m_referenceHeight = 720.0f;
        
        CanvasScaleMode m_scaleMode = CanvasScaleMode::ScaleWithScreenSize;
        
        // Cached values
        mutable f32 m_scaleFactor = 1.0f;
        mutable bool m_scaleFactorDirty = true;

    public:
        Canvas() = default;
        ~Canvas();

        // Prevent copying
        Canvas(const Canvas&) = delete;
        Canvas& operator=(const Canvas&) = delete;

        /**
         * @brief Set the reference resolution for scaling
         * @param width Reference width in pixels
         * @param height Reference height in pixels
         */
        void SetReferenceResolution(f32 width, f32 height);

        /**
         * @brief Get reference resolution
         */
        void GetReferenceResolution(f32& outWidth, f32& outHeight) const;

        /**
         * @brief Set the canvas scale mode
         */
        void SetScaleMode(CanvasScaleMode mode);
        CanvasScaleMode GetScaleMode() const { return m_scaleMode; }

        /**
         * @brief Set the canvas sort order (relative to other canvases)
         */
        void SetSortOrder(s32 order);
        s32 GetSortOrder() const { return m_sortOrder; }

        /**
         * @brief Get the current scale factor based on screen size
         */
        f32 GetScaleFactor() const;

        /**
         * @brief Get the canvas world rect (used as parent rect for root elements)
         */
        Rect GetCanvasRect() const;

        /**
         * @brief Update all UI elements
         * @param dt Delta time in seconds
         */
        void Update(f32 dt);

        /**
         * @brief Render all UI elements
         */
        void Render();

        /**
         * @brief Add a root element to the canvas
         */
        void AddElement(UIElement* element);

        /**
         * @brief Remove a root element from the canvas
         */
        void RemoveElement(UIElement* element);

        /**
         * @brief Get all root elements
         */
        const std::vector<UIElement*>& GetRootElements() const { return m_rootElements; }

        /**
         * @brief Create a child element of the specified type as root
         * @tparam T Type of element to create
         * @return Pointer to the created element
         */
        template<typename T>
        T* CreateChild()
        {
            static_assert(std::is_base_of<UIElement, T>::value, "T must inherit from UIElement");
            T* element = new T();
            AddElement(element);
            return element;
        }

        /**
         * @brief Convert screen coordinates to canvas world coordinates
         * @param screenX Screen X (from AEInputGetCursorPosition)
         * @param screenY Screen Y (from AEInputGetCursorPosition)
         * @param outWorldX Output world X
         * @param outWorldY Output world Y
         */
        void ScreenToCanvasPosition(s32 screenX, s32 screenY, f32& outWorldX, f32& outWorldY) const;

        /**
         * @brief Raycast to find the topmost element at a world position
         * @param worldX World X position
         * @param worldY World Y position
         * @return The topmost element containing the point, or nullptr
         */
        UIElement* Raycast(f32 worldX, f32 worldY) const;

        /**
         * @brief Sort elements by sorting order
         */
        void SortElements() const;

        /**
         * @brief Mark the canvas elements as needing a sort
         */
        void SetSortDirty() { m_sortDirty = true; }

    private:
        /**
         * @brief Recursive raycast through element hierarchy
         */
        UIElement* RaycastElement(UIElement* element, f32 worldX, f32 worldY) const;

        s32 m_sortOrder = 0;
        mutable bool m_sortDirty = true;
    };

} // namespace UI
