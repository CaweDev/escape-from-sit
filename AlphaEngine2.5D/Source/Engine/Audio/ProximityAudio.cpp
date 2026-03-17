#include "ProximityAudio.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Audio/SoundManager.hpp"

namespace Game
{
    void ProximityAudio::Update()
    {
        // 1. Safety check: Don't run math if we are missing references or the object is dead
        if (!playerTransform || !gameObject || !gameObject->active) return;

        // 2. Calculate distance between this object and the player
        float dist = Engine::Vector3::Distance(
            playerTransform->position,
            gameObject->transform->position
        );

        // 3. Update the SoundManager loop
        Engine::SoundManager::UpdateDistanceLoop(
            audioKey,
            dist,
            nearDistance,
            farDistance,
            maxVolume
        );
    }
}