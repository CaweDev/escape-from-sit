/* Start Header ************************************************************************/
/*!
\file		ProximityAudio.cpp
\author 	
\par	
\date	March, 29, 2026
\brief		[AUTO] ProximityAudio

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

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
