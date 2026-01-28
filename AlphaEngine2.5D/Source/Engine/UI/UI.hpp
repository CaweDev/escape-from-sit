#pragma once
/**
 * @file UI.hpp
 * @brief Master header for the UI system
 * 
 * Include this file to use all UI components.
 */

// Graphics Dependency
#include "Engine/Graphics/Texture2D.hpp"

// Foundation
#include "RectTransform.hpp"
#include "UIElement.hpp"
#include "Canvas.hpp"
#include "UIManager.hpp"

// Components (Phase 2)
#include "Image.hpp"
#include "Text.hpp"
#include "Panel.hpp"

// Interactivity (Phase 3)
#include "Button.hpp"
#include "UIEventSystem.hpp"

// Layout (Phase 4)
#include "LayoutGroup.hpp"
#include "VerticalLayoutGroup.hpp"
#include "HorizontalLayoutGroup.hpp"

// Gameplay UI
#include "Crosshair.hpp"
#include "InteractionPrompt.hpp"
#include "Minimap.hpp"
#include "Map.hpp"
