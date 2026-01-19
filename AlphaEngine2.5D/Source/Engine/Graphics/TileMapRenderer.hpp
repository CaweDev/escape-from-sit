/**
 * @file TileMapRenderer.hpp
 * @brief Component for rendering TileMaps using raycasting
 */
#pragma once

#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Graphics/IRenderer.hpp"
#include "Engine/Graphics/Color.hpp"
#include "Engine/Graphics/Sprite.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "AEEngine.h"
#include <cmath>
#include <memory>
#include <vector>

namespace Engine {

	/**
	 * @class TileMapRenderer
	 * @brief Renders walls, floor, and ceiling using DDA raycasting algorithm
	 */
	class TileMapRenderer : public IRenderer {
	public:
		REQUIRE_COMPONENT(TileMap);

		/** Field of view in degrees */
		f32 fov = 75.0f;

		/** Wall colors for different sides (used as fallback/tint if no sprite) */
		Color wallColorNS = Color(0.8f, 0.2f, 0.2f, 1.0f);
		Color wallColorEW = Color(0.6f, 0.15f, 0.15f, 1.0f);

		/** Sky and floor colors (used as fallback if no sprite) */
		Color ceilingColor = Color(0.2f, 0.2f, 0.4f, 1.0f);
		Color floorColor = Color(0.3f, 0.3f, 0.3f, 1.0f);

		TileMapRenderer();
		~TileMapRenderer();

		void Render() override;

		// Sprite setters
		void SetWallSprite(std::shared_ptr<Sprite> sprite) { wallSprite = sprite; }
		void SetCeilingSprite(std::shared_ptr<Sprite> sprite) { ceilingSprite = sprite; }
		void SetFloorSprite(std::shared_ptr<Sprite> sprite) { floorSprite = sprite; }

		// Sprite getters
		std::shared_ptr<Sprite> GetWallSprite() const { return wallSprite; }
		std::shared_ptr<Sprite> GetCeilingSprite() const { return ceilingSprite; }
		std::shared_ptr<Sprite> GetFloorSprite() const { return floorSprite; }

	private:
		AEGfxVertexList* columnMesh = nullptr;

		std::shared_ptr<Sprite> wallSprite = nullptr;
		std::shared_ptr<Sprite> ceilingSprite = nullptr;
		std::shared_ptr<Sprite> floorSprite = nullptr;

		// Rendering resolution constants
		static constexpr int COLUMN_STEP = 2;
		static constexpr int ROW_STEP = 8;
		static constexpr int MAX_TRIANGLES_PER_MESH = 8000;

		// Helper struct for raycast result
		struct RaycastResult {
			f32 perpWallDist;
			f32 wallX;  // Where on the wall the ray hit (0-1)
			int side;   // 0 = NS, 1 = EW
			f32 drawStart;
			f32 drawEnd;
		};

		// Shared column data to avoid redundant work
		struct ColumnInfo {
			RaycastResult ray;
			f32 rayDirX, rayDirY; // Store these for floor/ceiling calc
			f32 screenX, screenX2;
			f32 screenY, screenY2;
			f32 vTop, vBottom;
			f32 fogFactor;
		};

		void CreateColumnMesh();
		
		// Raycast helper
		RaycastResult CastRay(const Vector2& pos, f32 rayDirX, f32 rayDirY, f32 camZ, 
			int screenHeight, TileMap* map);

		// Separate render functions
		void RenderCeiling(const Vector2& pos, f32 camZ, int screenWidth, int screenHeight, 
			const std::vector<ColumnInfo>& columns);
		void RenderFloor(const Vector2& pos, f32 camZ, int screenWidth, int screenHeight, 
			const std::vector<ColumnInfo>& columns);
		void RenderWalls(int screenHeight, const std::vector<ColumnInfo>& columns);

		// Column drawing helpers
		void DrawColumn(f32 x, f32 y, f32 h, const Color& color, int sw, int sh);
	};

} // namespace Engine
