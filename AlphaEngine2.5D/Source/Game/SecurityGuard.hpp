#pragma once
#include "Engine/Core/Component.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include <vector>
#include <queue>
#include <cstdlib>
#include <ctime>
namespace Game {
	
	class SecurityGuard : public Engine::Component {
	public:
		Engine::TileMap* tileMap = nullptr;
		Engine::Transform* playerTf = nullptr;
		Player* playerData = nullptr;

		float patrolSpeed = 1.5f;
		float chaseSpeed = 2.4f;
		float arriveDistance = 0.10f;

		float detectRadius = 4.0f;
		float catchRadius = 0.7f;

		float searchDuration = 6.0f;
		int SearchRoamRadius = 3;

		/*Engine::Vector2 pointA;
		Engine::Vector2 pointB;*/
		float speed = 1.5f;
		
		
		void Initialize() override;
		void Update() override;

	private:
		enum class GuardState{Patrol,Chase,Search};
		GuardState state = GuardState::Patrol;

		std::vector<Engine::Vector2> path; //store path 
		int pathIndex = 0;

		static bool seeded;
		Engine::Vector2 lastSeenPos = Engine::Vector2(0, 0);
		bool hasLastSeen = false;

		float searchTimer = 0.0f;
		float repathTimer = 0.0f;



		void BuildPathBFS();
		int iD(int x, int y) const {
			return y *tileMap->width + x;
		}
		Engine::Vector2 WorldFromTile(int x, int y)const {
			return Engine::Vector2((float)x + 0.5f, (float)y + 0.5f);

		}


		/*bool goingtonextPoint = true;*/
	};
}

/*
	pseudo code
	pick random tile
	breadth first search
	convert bfs search into path
	guard follow it
	when reach pick new dest
	repeat bfs agan
*/