#pragma once
#include "Engine/Core/Component.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include <vector>

namespace Game {

	class CCTVCamera :public Engine::Component {
	public:
		Engine::TileMap* tile = nullptr;
		Engine::Transform* tf = nullptr;

		bool hasLastSeen = false;
		Engine::Vector2 lastSeenTile = Engine::Vector2(-1, -1);
		Engine::Vector2 lastSeenWorld = Engine::Vector2(0, 0);
		int id = 0;

	
		float detectRadius = 3.0f;
		float coneWidth = 70.0f;
		float maxRange = 6.0f;
		bool needLOS = true;

	
		bool showVision = true;  
		int  rayCount = 60;       
		float rayStepPerUnit = 12.0f; 

		const std::vector<Engine::Vector2>& GetVisionPolygon() const { return m_visionPoly; }


		bool IsTileVisible(int tx, int ty) const;

		void Update() override;

	private:
		void RecordLastSeen();
		bool InFov(float dx, float dy) const;
		bool HasLineOfSighttoPlayer() const;

		
		bool RaycastTileLOS(Engine::Vector2 from, Engine::Vector2 to) const;

	
		void BuildVisionPolygon();
		Engine::Vector2 RaycastToWall(Engine::Vector2 from, Engine::Vector2 dir, float maxDist) const;

	private:
		std::vector<Engine::Vector2> m_visionPoly; 
	};

}