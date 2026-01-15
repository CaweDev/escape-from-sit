/**
 * @file IRenderer.hpp
 * @brief Base component for all renderable components
 */
#pragma once

#include "../Core/Component.hpp"
#include "../Core/Transform.hpp"

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
};

} // namespace Engine
