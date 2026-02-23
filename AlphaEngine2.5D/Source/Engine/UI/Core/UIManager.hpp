#pragma once
/**
 * @file UIManager.hpp
 * @brief Singleton managing all UI canvases, input, and shared resources
 */

#include "AEEngine.h"
#include "Engine/UI/Core/Canvas.hpp"
#include "Engine/UI/Components/Image.hpp"
#include <vector>
#include <functional>

namespace UI
{
    /**
     * @class UIManager
     * @brief Singleton that manages all UI managers, services, rendering and input
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
         * @brief Notify UIManager that canvases need to be sorted
         */
        void SetCanvasesSortDirty() { m_canvasesSortDirty = true; }

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

        // ===== Screen Fade API =====

        /**
         * @brief Fade screen to black (transparent -> opaque)
         * @param duration Fade duration in seconds
         * @param onComplete Optional callback when fade completes
         */
        void FadeIn(f32 duration, std::function<void()> onComplete = nullptr);

        /**
         * @brief Fade screen from black (opaque -> transparent)
         * @param duration Fade duration in seconds
         * @param onComplete Optional callback when fade completes
         */
        void FadeOut(f32 duration, std::function<void()> onComplete = nullptr);

        /**
         * @brief Check if a fade is currently in progress
         */
        bool IsFading() const { return m_fading; }

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

        bool m_canvasesSortDirty = true;
        void SortCanvases();

        // ===== Screen Fade internals =====
        Canvas* m_fadeCanvas = nullptr;
        Image*  m_fadeImage  = nullptr;

        bool  m_fading = false;
        f32   m_fadeAlpha = 0.0f;
        f32   m_fadeFrom = 0.0f;
        f32   m_fadeTo = 0.0f;
        f32   m_fadeDuration = 0.0f;
        f32   m_fadeElapsed = 0.0f;
        std::function<void()> m_fadeCallback;

        void InitFadeOverlay();
        void UpdateFade(f32 dt);
    };

} // namespace UI
