#include "CCTVCamera.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include <cmath>

namespace Game {

	void CCTVCamera::Update() {
		if (!tile || !tf || !transform) {
			return;
		}

		float distanceX = tf->position.x - transform->position.x;
		float distanceY = tf->position.y - transform->position.y;

		if ((distanceX * distanceX + distanceY * distanceY) <= detectRadius * detectRadius) {
			RecordLastSeen();
		}
	}
	void CCTVCamera::RecordLastSeen() {
		int tileX = (int)tf->position.x;
		int tileY = (int)tf->position.y;

		hasLastSeen = true;
		lastSeenTile = Engine::Vector2((float)tileX, (float)tileY);
		lastSeenWorld = Engine::Vector2((float)tileX + 0.5f, (float)tileY + 0.5f);
	}



}