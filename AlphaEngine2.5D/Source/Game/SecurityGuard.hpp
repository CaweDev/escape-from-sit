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
		/*Engine::Vector2 pointA;
		Engine::Vector2 pointB;*/
		float speed = 1.5f;
		float arriveDistance = 0.1f;
		
		void Initialize() override;
		void Update() override;

	private:
		std::vector<Engine::Vector2> path; //store path 
		int pathIndex = 0;
		static bool seeded;
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