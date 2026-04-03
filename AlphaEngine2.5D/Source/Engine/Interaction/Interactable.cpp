/**
 * @file Interactable.cpp
 * @brief Implementation of Interactable component
 */
#include "Interactable.hpp"

namespace Engine {


void Interactable::Interact() {
    if (onInteract) {
        onInteract();
    }
}

} // namespace Engine
