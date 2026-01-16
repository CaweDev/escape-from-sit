#pragma once
#include "Engine/Core/Component.hpp"
#include "Engine/Math/Vector2.hpp"

namespace Game {

	class SecurityGuard : public Engine::Component {
	public:
		Engine::Vector2 pointA;
		Engine::Vector2 pointB;
		float speed = 1.5f;
		float arriveDistance = 0.1f;
		
		void Initialize() override;
		void Update() override;

	private:
		bool goingtonextPoint = true;
	};
}