/**
 * @file IRenderer.cpp
 * @brief Implementation of IRenderer class
 */
#include "IRenderer.hpp"

namespace Engine {

/*!
 * \brief [AUTO] GetSortingKey
 * \return 
 */
f32 IRenderer::GetSortingKey() const {
    if (!transform) return 0.0f;
    return transform->position.z;
}

} // namespace Engine
