/*!
 * \file AlphaEngine2.5D\Source\Game\Puzzle\PuzzleObject.cpp
 */

#include "PuzzleObject.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Core/Time.hpp"
#include <cmath>

namespace Game {

    void PuzzleObject::Initialize()
    {
        // Save the initial Z height exactly as it was spawned from the JSON
        // so we can use it as an anchor for the floating animation.
        if (gameObject && gameObject->transform) {
            baseZ = gameObject->transform->position.z;
        }
    }

    void PuzzleObject::Update()
    {
        // If the puzzle is already completed and the book is collected, 
        // we can stop animating it.
        if (puzzleCompleted) {
            return;
        }

        // --- Visual Juice: Floating / Bobbing Animation ---
        if (gameObject && gameObject->transform) {
            float hoverSpeed = 3.0f;   // How fast it bobs up and down
            float hoverHeight = 0.05f; // How high/low it goes

            // Calculate the new Z position using a sine wave based on time,
            // anchored strictly to our baseZ!
            float newZ = baseZ + (std::sin(Engine::Time::time * hoverSpeed) * hoverHeight);

            gameObject->transform->position.z = newZ;
        }
    }

} // namespace Game