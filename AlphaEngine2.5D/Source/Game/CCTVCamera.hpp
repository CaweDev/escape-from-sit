#pragma once
#include "Engine/Core/Component.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/TileMap/TileMap.hpp"

namespace Game {

	class CCTVCamera :public Engine::Component {
	public:
		Engine::TileMap* tile = nullptr;
		Engine::Transform* tf = nullptr;


		float detectRadius = 3.0f;
		bool hasLastSeen = false;
		Engine::Vector2 lastSeenTile = Engine::Vector2(-1, -1);
		Engine::Vector2 lastSeenWorld = Engine::Vector2(0, 0);
		int id = 0;
		void Update() override;


	private:
		void RecordLastSeen();
	};
}