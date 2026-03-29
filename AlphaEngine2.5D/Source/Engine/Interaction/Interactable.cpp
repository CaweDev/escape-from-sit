/**
 * @file Interactable.cpp
 * @brief Implementation of Interactable component
 */
#include "Interactable.hpp"

namespace Engine {

/*!
 * \brief [AUTO] Interact
 */
void Interactable::Interact() {
    if (onInteract) {
        onInteract();
    }
}

} // namespace Engine
