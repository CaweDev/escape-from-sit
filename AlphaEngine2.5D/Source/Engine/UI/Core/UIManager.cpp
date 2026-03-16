/**
 * @file UIManager.cpp
 * @brief UIManager implementation
 */

#include "Engine/UI/Core/UIManager.hpp"
#include "Engine/UI/Components/Image.hpp"
#include "Engine/Graphics/Texture2D.hpp"
#include "Engine/Graphics/Color.hpp"
#include "Engine/UI/Core/UIEventSystem.hpp"
#include "Engine/UI/Core/UIElement.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/UI/Helpers/DialogService.hpp"
#include "Engine/UI/Helpers/FadeService.hpp"
#include <algorithm>

namespace UI
{
    // Static member initialization
    UIManager* UIManager::s_instance = nullptr;
    AEGfxVertexList* UIManager::s_quadMesh = nullptr;

    bool UIManager::Initialize()
    {
        if (s_instance)
        {
            return true; // Already initialized
        }

        s_instance = new UIManager();
		s_instance->LoadDefaultFont("Assets/liberation-mono.ttf", 16);

        // Create shared resources
        CreateQuadMesh();
        Engine::Texture2D::InitializeStatics();

        // Initialize Dialog Service (it may create canvases)
        UI::DialogService::Initialize();

        // Initialize Fade Service
        FadeService::Initialize();

        return true;
    }

    void UIManager::Shutdown()
    {
        if (!s_instance) return;

        // Cleanup Dialog Service (must happen before UIManager destroys canvases)
        UI::DialogService::Shutdown();

        // Cleanup Fade Service
        FadeService::Shutdown();

        // Destroy all canvases (including fade canvas)
        for (auto* canvas : s_instance->m_canvases)
        {
            delete canvas;
        }
        s_instance->m_canvases.clear();

        // Destroy default font
        if (s_instance->m_defaultFont >= 0)
        {
            AEGfxDestroyFont(s_instance->m_defaultFont);
            s_instance->m_defaultFont = -1;
        }

        // Cleanup shared resources
        if (s_quadMesh)
        {
            AEGfxMeshFree(s_quadMesh);
            s_quadMesh = nullptr;
        }

        Engine::Texture2D::ShutdownStatics();

        delete s_instance;
        s_instance = nullptr;
    }

    void UIManager::CreateQuadMesh()
    {
        if (s_quadMesh) return;

        // Create a 1x1 quad centered at origin
        AEGfxMeshStart();

        // Triangle 1: bottom-left, bottom-right, top-left
        AEGfxTriAdd(
            -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,  // bottom-left
             0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,  // bottom-right
            -0.5f,  0.5f, 0xFFFFFFFF, 0.0f, 0.0f   // top-left
        );

        // Triangle 2: bottom-right, top-right, top-left
        AEGfxTriAdd(
             0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,  // bottom-right
             0.5f,  0.5f, 0xFFFFFFFF, 1.0f, 0.0f,  // top-right
            -0.5f,  0.5f, 0xFFFFFFFF, 0.0f, 0.0f   // top-left
        );

        s_quadMesh = AEGfxMeshEnd();
    }

    void UIManager::Update(f32 dt)
    {
        // Update fade overlay animation
        FadeService::Update(dt);

        // Process input first
        ProcessInput();

        // Update all canvases
        for (auto* canvas : m_canvases)
        {
            canvas->Update(dt);
        }
    }

    void UIManager::Render()
    {
        // Sort canvases if needed
        SortCanvases();

        // Standard Center-Origin World for UI
        AEGfxSetCamPosition(0.0f, 0.0f);

        // Render all canvases in order
        for (auto* canvas : m_canvases)
        {
            canvas->Render();
        }
    }

    void UIManager::SortCanvases()
    {
        if (m_canvasesSortDirty && !m_canvases.empty())
        {
            std::stable_sort(m_canvases.begin(), m_canvases.end(),
                [](const Canvas* a, const Canvas* b) {
                    return a->GetSortOrder() < b->GetSortOrder();
                });
            m_canvasesSortDirty = false;
        }
    }

    Canvas* UIManager::CreateCanvas()
    {
        Canvas* canvas = new Canvas();
        m_canvases.push_back(canvas);
        return canvas;
    }

    void UIManager::DestroyCanvas(Canvas* canvas)
    {
        if (!canvas) return;

        auto it = std::find(m_canvases.begin(), m_canvases.end(), canvas);
        if (it != m_canvases.end())
        {
            m_canvases.erase(it);
            delete canvas;
        }
    }

    bool UIManager::LoadDefaultFont(const char* fontPath, s32 fontSize)
    {
        if (m_defaultFont >= 0)
        {
            AEGfxDestroyFont(m_defaultFont);
        }

        m_defaultFont = AEGfxCreateFont(fontPath, fontSize);
        return m_defaultFont >= 0;
    }

    void UIManager::ScreenToWorld(s32 screenX, s32 screenY, f32& outWorldX, f32& outWorldY)
    {
        f32 screenWidth = static_cast<f32>(Engine::Application::GetWindowWidth());
        f32 screenHeight = static_cast<f32>(Engine::Application::GetWindowHeight());
 
        // Screen coordinates: (0,0) at top-left, Y increases downward
        // World coordinates: (0,0) at center, Y increases upward
        outWorldX = static_cast<f32>(screenX) - screenWidth * 0.5f;
        outWorldY = screenHeight * 0.5f - static_cast<f32>(screenY);
    }

    void UIManager::OnElementDestroyed(UIElement* element)
    {
        if (m_hoveredElement == element)
        {
            m_hoveredElement = nullptr;
        }
        if (m_pressedElement == element)
        {
            m_pressedElement = nullptr;
        }
    }

    void UIManager::ProcessInput()
    {
        // Ensure canvas order is up-to-date before hit testing
        SortCanvases();

        // Get mouse position
        s32 mouseX, mouseY;
        AEInputGetCursorPosition(&mouseX, &mouseY);

        f32 worldX, worldY;
        ScreenToWorld(mouseX, mouseY, worldX, worldY);

        // Raycast to find element under mouse
        UIElement* hitElement = RaycastAll(worldX, worldY);

        // Prepare event data
        PointerEventData eventData;
        eventData.worldX = worldX;
        eventData.worldY = worldY;
        eventData.screenX = mouseX;
        eventData.screenY = mouseY;
        eventData.isPressed = AEInputCheckCurr(AEVK_LBUTTON);
        eventData.target = hitElement;

        // Handle hover state changes
        if (hitElement != m_hoveredElement)
        {
            // Dispatch OnPointerExit to old element
            if (m_hoveredElement)
            {
                auto* exitHandler = dynamic_cast<IPointerExitHandler*>(m_hoveredElement);
                if (exitHandler)
                {
                    exitHandler->OnPointerExit(eventData);
                }
            }

            // Dispatch OnPointerEnter to new element
            if (hitElement)
            {
                auto* enterHandler = dynamic_cast<IPointerEnterHandler*>(hitElement);
                if (enterHandler)
                {
                    enterHandler->OnPointerEnter(eventData);
                }
            }

            m_hoveredElement = hitElement;
        }

        // Handle mouse button using Input class edge detection
        bool mouseJustPressed  = Engine::Input::GetMouseButtonDown(0);
        bool mouseJustReleased = Engine::Input::GetMouseButtonUp(0);

        if (mouseJustPressed)
        {
            // Mouse just pressed — record which element was under the cursor
            m_pressedElement = m_hoveredElement;
            if (m_pressedElement)
            {
                auto* downHandler = dynamic_cast<IPointerDownHandler*>(m_pressedElement);
                if (downHandler)
                {
                    downHandler->OnPointerDown(eventData);
                }
            }
        }

        if (mouseJustReleased)
        {
            // Mouse just released
            if (m_pressedElement)
            {
                auto* upHandler = dynamic_cast<IPointerUpHandler*>(m_pressedElement);
                if (upHandler)
                {
                    upHandler->OnPointerUp(eventData);
                }

                // Click only if released on the SAME element that received the press
                if (m_pressedElement == m_hoveredElement)
                {
                    auto* clickHandler = dynamic_cast<IPointerClickHandler*>(m_pressedElement);
                    if (clickHandler)
                    {
                        clickHandler->OnPointerClick(eventData);
                    }
                }
            }
            m_pressedElement = nullptr;
        }
    }

    UIElement* UIManager::RaycastAll(f32 worldX, f32 worldY) const
    {
        // Raycast through canvases in reverse order (later canvases are on top)
        for (auto it = m_canvases.rbegin(); it != m_canvases.rend(); ++it)
        {
            UIElement* hit = (*it)->Raycast(worldX, worldY);
            if (hit) return hit;
        }
        return nullptr;
    }

} // namespace UI
