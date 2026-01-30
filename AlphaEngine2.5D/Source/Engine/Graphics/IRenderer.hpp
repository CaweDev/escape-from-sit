/**
 * @file IRenderer.hpp
 * @brief Base component for all renderable components
 */
#pragma once

#include "../Core/Component.hpp"
#include "../Core/Transform.hpp"
#include "Color.hpp"
#include "AEEngine.h"

namespace Engine {

/**
 * @class IRenderer
 * @brief Abstract base class for components that render to the screen
 */
class IRenderer : public Component {
public:
    virtual ~IRenderer() = default;

    /** Main render call (called by engine or specialized scenes) */
    virtual void Render() = 0;
    
    /** Get sorting key for depth sorting */
    virtual f32 GetSortingKey() const;

    /** If true, renders with highlight effect (used by InteractionController) */
    bool highlighted = false;

    /** Color added when highlighted (additive glow effect) */
    Color highlightColor = Color(0.2f, 0.2f, 0.0f, 0.0f);
};

} // namespace Engine
