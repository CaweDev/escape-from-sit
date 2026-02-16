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
#include <unordered_map>

namespace Engine {

	/**
	 * @class TileMapRenderer
	 * @brief Renders walls, floor, and ceiling using DDA raycasting algorithm
	 */
	class TileMapRenderer : public IRenderer {
	private:
		// Rendering resolution constants
		static constexpr int COLUMN_STEP = 8;
		static constexpr int ROW_STEP = 8;
		static constexpr int MAX_TRIANGLES_PER_MESH = 8000;

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
		
		/** Ensure TileMap renders first by returning max distance */
		f32 GetSortingKey() const override { return FLT_MAX; }

		// Type-indexed sprite setters (use int for tile type ID)
		void SetWallSprite(int tileType, Sprite* sprite) { wallSprites[tileType] = sprite; }
		void SetCeilingSprite(int tileType, Sprite* sprite) { ceilingSprites[tileType] = sprite; }
		void SetFloorSprite(int tileType, Sprite* sprite) { floorSprites[tileType] = sprite; }

		// Convenience setters for default sprite (type 1)
		void SetWallSprite(Sprite* sprite) { wallSprites[1] = sprite; }
		void SetCeilingSprite(Sprite* sprite) { ceilingSprites[1] = sprite; }
		void SetFloorSprite(Sprite* sprite) { floorSprites[1] = sprite; }

		// Type-indexed sprite getters
		Sprite* GetWallSprite(int tileType) const {
			auto it = wallSprites.find(tileType);
			return (it != wallSprites.end()) ? it->second : nullptr;
		}
		Sprite* GetCeilingSprite(int tileType) const {
			auto it = ceilingSprites.find(tileType);
			return (it != ceilingSprites.end()) ? it->second : nullptr;
		}
		Sprite* GetFloorSprite(int tileType) const {
			auto it = floorSprites.find(tileType);
			return (it != floorSprites.end()) ? it->second : nullptr;
		}

		// Convenience getters for default sprite (type 1)
		Sprite* GetWallSprite() const { return GetWallSprite(1); }
		Sprite* GetCeilingSprite() const { return GetCeilingSprite(1); }
		Sprite* GetFloorSprite() const { return GetFloorSprite(1); }

	private:
		AEGfxVertexList* columnMesh = nullptr;

		// Maps from tile type ID to sprite
		std::unordered_map<int, Sprite*> wallSprites;
		std::unordered_map<int, Sprite*> ceilingSprites;
		std::unordered_map<int, Sprite*> floorSprites;

		// Helper struct for raycast result
		struct RaycastResult {
			f32 perpWallDist;
			f32 wallX;  // Where on the wall the ray hit (0-1)
			int side;   // 0 = NS, 1 = EW
			int tileType; // The tile type that was hit
			f32 drawStart;
			f32 drawEnd;
		};

		// Shared column data to avoid redundant work
		struct ColumnInfo {
			RaycastResult ray;
			f32 rayDirX, rayDirY;   // Left side ray
			f32 rayDirX2, rayDirY2; // Right side ray
			f32 screenX, screenX2;
			f32 screenY, screenY2;
			f32 vTop, vBottom;
			f32 fogFactor;
			int fogKey; // Added for sorting-based batching
		};

		// Cached scanline data to avoid per-frame recalculation
		struct ScanlineInfo {
			f32 rowDist;      // Distance from camera to this scanline's floor/ceiling
			f32 aeY;          // Screen Y coordinate in AE space
		};

		// Cached scanline tables (indexed by screen Y)
		std::vector<ScanlineInfo> ceilingScanlines;  // For rows 0 to horizon
		std::vector<ScanlineInfo> floorScanlines;    // For rows horizon to screenHeight
		int cachedScreenHeight = 0;
		f32 cachedCamZ = -999.0f;
		int cachedHorizon = -999;
		f32 cachedFogDistance = -1.0f; // Track LUT validity
		std::vector<f32> fogLUT;
		std::vector<ColumnInfo> managedColumns; // Pre-allocated column buffer

		void CreateColumnMesh();
		
		// Precompute scanline distances for floor/ceiling
		void CacheScanlineDistances(int screenHeight, f32 camZ, int horizon);
		void UpdateFogLUT(f32 fogDistance);

		// Raycast helper
		RaycastResult CastRay(const Vector2& pos, f32 rayDirX, f32 rayDirY, f32 camZ, 
			int horizon, int screenHeight, TileMap* map);

		// Separate render functions
		void RenderCeiling(const Vector2& pos, f32 camZ, int horizon, int screenWidth, int screenHeight, 
			const std::vector<ColumnInfo>& columns, TileMap* map);
		void RenderFloor(const Vector2& pos, f32 camZ, int horizon, int screenWidth, int screenHeight, 
			const std::vector<ColumnInfo>& columns, TileMap* map);
		void RenderWalls(int horizon, int screenHeight, const std::vector<ColumnInfo>& columns);

		// Column drawing helpers
		void DrawColumn(f32 x, f32 y, f32 h, const Color& color, int sw, int sh);
	};

} // namespace Engine
