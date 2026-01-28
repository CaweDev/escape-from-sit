#pragma once
/**
 * @file UIManager.hpp
 * @brief Singleton managing all UI canvases, input, and shared resources
 */

#include "AEEngine.h"
#include "Canvas.hpp"
#include <vector>

namespace UI
{
    /**
     * @class UIManager
     * @brief Singleton that manages all UI rendering and input
     * 
     * Responsibilities:
     * - Owns the shared quad mesh for all UI rendering
     * - Manages active Canvas objects
     * - Processes mouse input and dispatches to UI elements
     * - Provides coordinate conversion utilities
     */
    class UIManager
    {
    private:
        // Singleton instance
        static UIManager* s_instance;

        // Shared 1x1 quad mesh for all UI rendering
        static AEGfxVertexList* s_quadMesh;

        // Active canvases
        std::vector<Canvas*> m_canvases;

        // Input state tracking
        UIElement* m_hoveredElement = nullptr;
        UIElement* m_pressedElement = nullptr;
        bool m_mouseWasPressed = false;

        // Default font
        s8 m_defaultFont = -1;

        // Private constructor (singleton)
        UIManager() = default;

    public:
        // Prevent copying
        UIManager(const UIManager&) = delete;
        UIManager& operator=(const UIManager&) = delete;

        /**
         * @brief Initialize the UI system
         * @return True if initialization succeeded
         */
        static bool Initialize();

        /**
         * @brief Shutdown the UI system and cleanup resources
         */
        static void Shutdown();

        /**
         * @brief Get the singleton instance
         * @return Pointer to the UIManager instance (nullptr if not initialized)
         */
        static UIManager* GetInstance() { return s_instance; }

        /**
         * @brief Get the shared quad mesh for UI rendering
         * @return Pointer to the 1x1 quad mesh
         */
        static AEGfxVertexList* GetQuadMesh() { return s_quadMesh; }

        /**
         * @brief Update all UI (call once per frame)
         * @param dt Delta time in seconds
         */
        void Update(f32 dt);

        /**
         * @brief Render all UI (call once per frame, after game rendering)
         */
        void Render();

        /**
         * @brief Create a new canvas
         * @return Pointer to the created canvas
         */
        Canvas* CreateCanvas();

        /**
         * @brief Destroy a canvas
         * @param canvas Canvas to destroy
         */
        void DestroyCanvas(Canvas* canvas);

        /**
         * @brief Get all active canvases
         */
        const std::vector<Canvas*>& GetCanvases() const { return m_canvases; }

        /**
         * @brief Get the default font handle
         * @return Font handle, or -1 if not loaded
         */
        s8 GetDefaultFont() const { return m_defaultFont; }


        /**
         * @brief Get the currently hovered UI element
         */
        UIElement* GetHoveredElement() const { return m_hoveredElement; }

        /**
         * @brief Convert screen coordinates to world coordinates
         * @param screenX Screen X (from AEInputGetCursorPosition)
         * @param screenY Screen Y (from AEInputGetCursorPosition)
         * @param outWorldX Output world X
         * @param outWorldY Output world Y
         */
        static void ScreenToWorld(s32 screenX, s32 screenY, f32& outWorldX, f32& outWorldY);
        
        /**
         * @brief Notify UIManager that an element is being destroyed
         * @param element Pointer to the element being destroyed
         */
        void OnElementDestroyed(UIElement* element);

    private:
        /**
         * @brief Load the default font
         * @param fontPath Path to TTF font file
         * @param fontSize Font size in pixels
         * @return True if loading succeeded
         */
        bool LoadDefaultFont(const char* fontPath, s32 fontSize);


        /**
         * @brief Create the shared quad mesh
         */
        static void CreateQuadMesh();

        /**
         * @brief Process mouse input and dispatch events
         */
        void ProcessInput();

        /**
         * @brief Raycast across all canvases to find hit element
         */
        UIElement* RaycastAll(f32 worldX, f32 worldY) const;
    };

} // namespace UI
