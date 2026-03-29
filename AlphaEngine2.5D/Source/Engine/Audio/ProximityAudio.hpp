/* Start Header ************************************************************************/
/*!
\file		ProximityAudio.hpp
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

#pragma once
#include "Engine/Core/Component.hpp"
#include <string>

// Forward declaration keeps the header clean and compile times fast!
namespace Engine {
    class Transform;
}

namespace Game
{
    class ProximityAudio : public Engine::Component
    {
    public:
        // Pointers and settings
        Engine::Transform* playerTransform = nullptr;
        std::string audioKey = "";

        // Tuning variables
        float nearDistance = 2.0f;
        float farDistance = 6.0f;
        float maxVolume = 0.5f;

        // Engine lifecycle overrides
        void Update() override;
    };
}
