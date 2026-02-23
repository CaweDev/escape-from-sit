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
#include "Engine/UI/Core/RectTransform.hpp"
#include "Engine/UI/Core/UIElement.hpp"
#include "Engine/UI/Core/Canvas.hpp"
#include "Engine/UI/Core/UIManager.hpp"

// Components (Phase 2)
#include "Engine/UI/Components/Image.hpp"
#include "Engine/UI/Components/SpriteSheetImage.hpp"
#include "Engine/UI/Components/Text.hpp"
#include "Engine/UI/Components/Panel.hpp"

// Interactivity (Phase 3)
#include "Engine/UI/Components/Button.hpp"
#include "Engine/UI/Core/UIEventSystem.hpp"

// Layout (Phase 4)
#include "Engine/UI/Layout/LayoutGroup.hpp"
#include "Engine/UI/Layout/VerticalLayoutGroup.hpp"
#include "Engine/UI/Layout/HorizontalLayoutGroup.hpp"

// Helpers
#include "Engine/UI/Helpers/NineSliceRenderer.hpp"
#include "Engine/UI/Helpers/DialogService.hpp"
#include "Engine/UI/Helpers/FadeService.hpp"

// UI GameObjects
#include "Engine/UI/GameObject/ConfirmationDialog.hpp"

// Gameplay UI
#include "Engine/UI/Specialized/Crosshair.hpp"
#include "Engine/UI/Specialized/InteractionPrompt.hpp"
#include "Engine/UI/Specialized/Minimap.hpp"
#include "Engine/UI/Specialized/Map.hpp"

