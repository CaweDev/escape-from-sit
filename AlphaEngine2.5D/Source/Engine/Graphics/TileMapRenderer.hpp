/**
 * @file TileMapRenderer.hpp
 * @brief 3D raycasting renderer for tile-based maps.
 *
 * Renders a first-person 3D view using the DDA raycasting algorithm, similar
 * to classic games like Wolfenstein 3D and DOOM. Each frame it:
 *
 *   1. Casts one ray per COLUMN_STEP pixels across the screen width (the "column pass").
 *   2. Renders ceiling and floor into scaled CPU framebuffers, then uploads them
 *      as a single GPU texture each via AEGfxTextureLoadFromMemory.
 *   3. Renders textured wall columns as GPU triangle strips, batched by tile type
 *      and fog key to minimise draw calls.
 *
 * @section perf Performance notes
 * - COLUMN_STEP controls the horizontal raycast granularity. Higher = fewer rays = faster,
 *   but columns appear wider and "blocky". Default 8.
 * - fbScale controls the floor/ceiling framebuffer resolution relative to screen size.
 *   0.25 = quarter-res (very fast, pixelated), 0.5 = half-res (good balance), 1.0 = full-res.
 * - Fog is applied via a pre-computed look-up table (fogLUT) to avoid per-pixel expf() calls.
 *   The LUT is rebuilt only when Camera::main's fogDistance changes.
 *
 * @section pipeline Render pipeline order
 *   Ceiling (framebuffer quad) → Floor (framebuffer quad) → Walls (GPU strips, on top)
 *   Sprites are drawn after this renderer by SpriteRenderer, using ZBuffer for occlusion.
 */
#pragma once

#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Graphics/IRenderer.hpp"
#include "Engine/Graphics/Color.hpp"
#include "Engine/Graphics/Sprite.hpp"
#include "Engine/Graphics/TextureUtils.hpp"
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
	 * @brief First-person 3D renderer that raycasts against a TileMap.
	 *
	 * Attach this component to any GameObject that also has a TileMap component.
	 * Assign floor, ceiling, and wall sprites by tile-type ID using the Set*Sprite
	 * family of methods.
	 *
	 * @par Render order
	 * Returns FLT_MAX from GetSortingKey() so the IRenderer system always draws
	 * this renderer before all other renderers (sprites, UI, etc.).
	 *
	 * @par Floor/Ceiling rendering
	 * Floor and ceiling are rendered into CPU-side RGBA framebuffers at
	 * (screenWidth * fbScale) × (screenHeight * fbScale) resolution, then uploaded
	 * to the GPU as a transient texture each frame and drawn as a single full-screen quad.
	 * Pixel data is loaded once from disk via Windows Imaging Component (WIC) when
	 * SetFloorSprite / SetCeilingSprite is called.
	 *
	 * @par Wall rendering
	 * Walls use the traditional AlphaEngine AEGfxTriAdd pipeline. Column strips are
	 * sorted by tile type and fog key so consecutive same-type columns can be batched
	 * into a single mesh, keeping GPU draw calls low.
	 */
	class TileMapRenderer : public IRenderer {
	public:
		struct TileSprites {
			Sprite* wall = nullptr;
			Sprite* ceiling = nullptr;
			Sprite* floor = nullptr;
			Sprite* door = nullptr;
		};

	private:
		// ---------------------------------------------------------------
		// Rendering resolution constants for WALLS only.
		// Changing COLUMN_STEP affects how many rays are cast per frame
		// and therefore the horizontal resolution of the wall pass.
		// ---------------------------------------------------------------
		/** Number of screen pixels per raycasted column. Higher = faster but blockier. */
		static constexpr int COLUMN_STEP = 8;
		/** (Legacy, unused for floor/ceiling) Row step for old strip-based rendering. */
		static constexpr int ROW_STEP = 8;
		/** Maximum triangles per AEGfx mesh batch before forcing a flush. */
		static constexpr int MAX_TRIANGLES_PER_MESH = 8000;

		// ---------------------------------------------------------------
		// Framebuffer quality setting (public, adjustable at runtime)
		// ---------------------------------------------------------------

		/**
		 * @brief Floor/ceiling framebuffer resolution relative to screen size.
		 *
		 * The framebuffer is rendered at (screenWidth × fbScale, screenHeight × fbScale)
		 * and stretched to fill the screen by the GPU. Lower values reduce CPU pixel-fill
		 * work and GPU texture upload bandwidth at the cost of visual quality.
		 *
		 * Recommended values:
		 *   - 0.25 → quarter-res (400×225 @ 1600×900) — fastest, noticeably pixelated
		 *   - 0.5  → half-res   (800×450 @ 1600×900) — good balance
		 *   - 1.0  → full-res   (1600×900)           — matches wall quality, slowest
		 */
		f32 fbScale = 0.2f;

	public:
		REQUIRE_COMPONENT(TileMap);

		/** @brief Horizontal field-of-view in degrees. Default 75. */
		f32 fov = 75.0f;

		/** @brief Wall tint for north/south facing walls (used if no wall sprite is set). */
		Color wallColorNS = Color(0.8f, 0.2f, 0.2f, 1.0f);
		/** @brief Wall tint for east/west facing walls (used if no wall sprite is set). */
		Color wallColorEW = Color(0.6f, 0.15f, 0.15f, 1.0f);

		/** @brief Fallback ceiling color (used if no ceiling sprite is set). */
		Color ceilingColor = Color(0.2f, 0.2f, 0.4f, 1.0f);
		/** @brief Fallback floor color (used if no floor sprite is set). */
		Color floorColor = Color(0.3f, 0.3f, 0.3f, 1.0f);

		TileMapRenderer();
		~TileMapRenderer();

		/** @brief Performs the full raycasting render pass for this frame. */
		void Render() override;

		/**
		 * @brief Returns FLT_MAX so TileMapRenderer always sorts first.
		 *
		 * The raycasting pass must execute first because it populates the ZBuffer
		 * that SpriteRenderer reads to perform per-column occlusion culling.
		 */
		f32 GetSortingKey() const override { return FLT_MAX; }

		// ---------------------------------------------------------------
		// Sprite assignment
		// ---------------------------------------------------------------

		/**
		 * @brief Assign all sprites for a specific tile type ID.
		 * @param tileType  Integer tile type returned by TileMap::GetTile().
		 */
		void SetTileSprites(int tileType, Sprite* wall, Sprite* ceiling, Sprite* floor, Sprite* door = nullptr);

		/** @brief Convenience: sets the default (type 1) wall sprite. */
		void SetWallSprite(Sprite* sprite) { tileSprites[1].wall = sprite; }
		/** @brief Convenience: sets the default (type 1) ceiling sprite. */
		void SetCeilingSprite(Sprite* sprite);
		/** @brief Convenience: sets the default (type 1) floor sprite. */
		void SetFloorSprite(Sprite* sprite);
		/** @brief Convenience: sets the default (type 1) door sprite. */
		void SetDoorSprite(Sprite* sprite) { tileSprites[1].door = sprite; }

		// ---------------------------------------------------------------
		// Sprite retrieval
		// ---------------------------------------------------------------

		/** @brief Returns all sprites assigned to @p tileType. */
		TileSprites GetTileSprites(int tileType) const {
			auto it = tileSprites.find(tileType);
			return (it != tileSprites.end()) ? it->second : TileSprites{};
		}

		/** @brief Returns the wall sprite assigned to @p tileType, or nullptr. */
		Sprite* GetWallSprite(int tileType) const {
			auto it = tileSprites.find(tileType);
			return (it != tileSprites.end()) ? it->second.wall : nullptr;
		}
		/** @brief Returns the ceiling sprite assigned to @p tileType, or nullptr. */
		Sprite* GetCeilingSprite(int tileType) const {
			auto it = tileSprites.find(tileType);
			return (it != tileSprites.end()) ? it->second.ceiling : nullptr;
		}
		/** @brief Returns the floor sprite assigned to @p tileType, or nullptr. */
		Sprite* GetFloorSprite(int tileType) const {
			auto it = tileSprites.find(tileType);
			return (it != tileSprites.end()) ? it->second.floor : nullptr;
		}
		/** @brief Returns the door sprite assigned to @p tileType, or nullptr. */
		Sprite* GetDoorSprite(int tileType) const {
			auto it = tileSprites.find(tileType);
			return (it != tileSprites.end()) ? it->second.door : nullptr;
		}

		/** @brief Convenience: returns the default (type 1) wall sprite, or nullptr. */
		Sprite* GetWallSprite() const { return GetWallSprite(1); }
		/** @brief Convenience: returns the default (type 1) ceiling sprite, or nullptr. */
		Sprite* GetCeilingSprite() const { return GetCeilingSprite(1); }
		/** @brief Convenience: returns the default (type 1) floor sprite, or nullptr. */
		Sprite* GetFloorSprite() const { return GetFloorSprite(1); }
		/** @brief Convenience: returns the default (type 1) door sprite, or nullptr. */
		Sprite* GetDoorSprite() const { return GetDoorSprite(1); }

		// Path overlay support - highlights tiles on the floor
		void SetPathTiles(const std::vector<Vector2>& waypoints) { pathWaypoints = waypoints; }
		void ClearPathTiles() { pathWaypoints.clear(); }

		

		

	private:
		// ---------------------------------------------------------------
		// GPU mesh handles
		// ---------------------------------------------------------------
		/** 1×1 unit quad used by DrawColumn for color-only fallback walls. */
		AEGfxVertexList* columnMesh = nullptr;
		/** Full-screen unit quad (-0.5..0.5) used to draw floor/ceiling framebuffer textures. */
		AEGfxVertexList* fullscreenMesh = nullptr;

		// ---------------------------------------------------------------
		// Sprite map: tile type ID → TileSprites
		// ---------------------------------------------------------------
		std::unordered_map<int, TileSprites> tileSprites;

		// ---------------------------------------------------------------
		// DDA raycasting data structures
		// ---------------------------------------------------------------

		/**
		 * @brief Result from a single DDA ray cast against the tile map.
		 *
		 * Computed once per column block in the column pass, then reused for
		 * both wall rendering and floor/ceiling occlusion testing.
		 */
		struct RaycastResult {
			f32 perpWallDist;  ///< Perpendicular distance to the hit wall (used for Z-buffer and wall height).
			f32 wallX;         ///< World-space hit coordinate along the wall face (used as texture U).
			int side;          ///< Which axis was hit: 0 = X-axis (N/S wall), 1 = Y-axis (E/W wall).
			int tileType;      ///< Tile type integer at the hit cell (used to look up wall sprite).
			f32 drawStart;     ///< Screen-space Y pixel where the wall top begins (may be negative if clipped).
			f32 drawEnd;       ///< Screen-space Y pixel where the wall bottom ends (may exceed screen height).
		};

		/**
		 * @brief Per-column data cached after the column pass, shared by wall and floor/ceiling renderers.
		 *
		 * Each entry covers COLUMN_STEP screen pixels. Ray directions for the left and right
		 * edges of the column block are stored so that floor/ceiling rendering can interpolate
		 * across the block without recasting rays.
		 */
		struct ColumnInfo {
			RaycastResult ray;             ///< DDA result for the left edge of this column block.
			f32 rayDirX,  rayDirY;         ///< Ray direction at the left edge of the column block.
			f32 rayDirX2, rayDirY2;        ///< Ray direction at the right edge of the column block.
			f32 screenX,  screenX2;        ///< AE screen-space X extents of this column block (centred coords).
			f32 screenY,  screenY2;        ///< AE screen-space Y extents of the visible wall slice (centred coords).
			f32 vTop,     vBottom;         ///< Texture V coordinates for the top and bottom of the clipped wall slice.
			f32 fogFactor;                 ///< Pre-computed fog attenuation [0, 1] for this column's depth.
			int fogKey;                    ///< Quantised fog bucket (fogFactor × fog.precision) for batch sorting.
		};

		// ---------------------------------------------------------------
		// Per-frame cached state (recomputed in Render() each frame)
		// ---------------------------------------------------------------

		/** Half-plane length = tan(fov/2). Cached to avoid tanf() per frame. */
		f32 cachedPlaneLength = 0.0f;

		/**
		 * @brief Fog settings snapshotted from Camera::main once at the start of Render().
		 *
		 * Avoids repeated pointer dereferences and virtual calls in tight inner loops.
		 */
		struct FogSettings {
			bool enabled      = false;   ///< Whether fog is currently active.
			f32  distance     = 20.0f;   ///< Fog characteristic distance (camera units).
			f32  minVisibility = 0.2f;   ///< Minimum fog factor (prevents fully black far geometry).
			f32  maxVisibility = 1.0f;   ///< Maximum fog factor (1.0 = no darkening at close range).
			f32  precision    = 200.0f;  ///< Quantisation divisor for fog batching.
		} fog;

		/** Last fog distance used to build fogLUT; used to detect when a rebuild is needed. */
		f32 cachedFogDistance = -1.0f;

		/**
		 * @brief Pre-computed exponential fog look-up table.
		 *
		 * fogLUT[i] = exp(-i * 0.01 / fogDistance). Indexed by (distance × 100),
		 * covering distances 0–100 world units at 0.01-unit precision.
		 * Rebuilt automatically in Render() whenever the fog distance changes.
		 */
		std::vector<f32> fogLUT;

		/** Pre-allocated column buffer. Resized to the column count each frame. */
		std::vector<ColumnInfo> managedColumns;

		/** Identity matrix cached per-frame to avoid redundant constructions in wall rendering. */
		AEMtx33 identityMtx;

		// ---------------------------------------------------------------
		// Framebuffer rendering state (floor/ceiling CPU rasteriser)
		// Uses Engine::CpuTexture from TextureUtils.hpp
		// ---------------------------------------------------------------

		CpuTexture floorCpuTex;  ///< CPU pixel data for the default floor sprite (type 1).
		CpuTexture ceilCpuTex;   ///< CPU pixel data for the default ceiling sprite (type 1).

		/**
		 * @brief Combined per-frame RGBA pixel buffer for both floor and ceiling surfaces.
		 * Dimensions: fbWidth × fbHeight × 4 bytes.
		 */
		std::vector<u8> combinedFramebuffer;

		/** Current framebuffer width in pixels (= screenWidth × fbScale). */
		int fbWidth  = 0;
		/** Current framebuffer height in pixels (= screenHeight × fbScale). */
		int fbHeight = 0;
		
		// Path waypoints for floor overlay
		std::vector<Vector2> pathWaypoints;

		// ---------------------------------------------------------------
		// Private methods
		// ---------------------------------------------------------------

		/** @brief Creates the 1×1 unit quad mesh used by DrawColumn. Called once on first Render(). */
		void CreateColumnMesh();

		/** @brief Creates the full-screen quad mesh used to blit floor/ceiling framebuffers. Called once on first Render(). */
		void CreateFullscreenMesh();

		/**
		 * @brief Rebuilds the exponential fog look-up table for a new fog distance.
		 * @param fogDistance  New characteristic distance. Must be > 0.
		 */
		void UpdateFogLUT(f32 fogDistance);

		/**
		 * @brief Performs DDA raycasting for a single ray direction against the tile map.
		 *
		 * Uses the standard grid-traversal DDA algorithm:
		 *  - Computes initial X/Y side distances from the camera position.
		 *  - Steps through grid cells until a wall tile is found.
		 *  - Returns perpendicular wall distance, hit coordinate, and screen-space
		 *    draw extents corrected for camera height (camZ) and pitch (horizon).
		 *
		 * @param pos          Camera world-space XY position.
		 * @param rayDirX/Y    Normalised ray direction.
		 * @param camZ         Camera height in [0, 1] (0 = ground, 1 = ceiling).
		 * @param horizon      Screen-space Y of the horizon (screenHeight/2 + pitch).
		 * @param screenWidth  Current screen width in pixels.
		 * @param screenHeight Current screen height in pixels.
		 * @param map          TileMap to cast against.
		 * @return             Populated RaycastResult.
		 */
		RaycastResult CastRay(const Vector2& pos, f32 rayDirX, f32 rayDirY, f32 camZ,
			int horizon, int screenWidth, int screenHeight, TileMap* map);

		/**
		 * @brief Rasterises the ceiling into ceilFramebuffer and draws it as a single fullscreen quad.
		 *
		 * Iterates over the framebuffer at fbScale resolution, maps each pixel back to screen
		 * space, computes world-space UV via perspective projection, samples the CPU texture,
		 * applies fog, and writes RGBA to ceilFramebuffer. The buffer is then uploaded via
		 * AEGfxTextureLoadFromMemory and drawn with a scaled identity transform.
		 *
		 * @param pos          Camera world-space XY position.
		 * @param camZ         Camera height in [0, 1].
		 * @param horizon      Screen-space Y of the horizon.
		 * @param screenWidth  Screen width in pixels.
		 * @param screenHeight Screen height in pixels.
		 * @param columns      Column data from the column pass (provides per-column ray dirs + wall occlusion).
		 * @param map          TileMap (unused in this path, kept for API symmetry).
		 */
		void RenderCeiling(const Vector2& pos, f32 camZ, int horizon, int screenWidth, int screenHeight,
			const std::vector<ColumnInfo>& columns);

		/**
		 * @brief Rasterises the floor into floorFramebuffer and draws it as a single fullscreen quad.
		 *
		 * Mirrors RenderCeiling but iterates rows below the horizon. Includes an early-out
		 * optimisation that skips rows that are fully fogged (beyond fog.distance × 4.6).
		 *
		 * @param pos          Camera world-space XY position.
		 * @param camZ         Camera height in [0, 1].
		 * @param horizon      Screen-space Y of the horizon.
		 * @param screenWidth  Screen width in pixels.
		 * @param screenHeight Screen height in pixels.
		 * @param columns      Column data from the column pass.
		 * @param map          TileMap (unused in this path, kept for API symmetry).
		 */
		void RenderFloor(const Vector2& pos, f32 camZ, int horizon, int screenWidth, int screenHeight,
			const std::vector<ColumnInfo>& columns);

		/**
		 * @brief Draws the wall columns using AEGfxTriAdd batched by tile type and fog key.
		 *
		 * Requires the columns vector to be pre-sorted by (tileType, fogKey) so that
		 * same-appearance columns are consecutive and can share a single AEGfx mesh.
		 * Flushes the current mesh whenever the tile type or fog key changes, or when
		 * MAX_TRIANGLES_PER_MESH is reached.
		 *
		 * @param horizon      Screen-space Y of the horizon (used for vertical culling reference).
		 * @param screenHeight Screen height in pixels.
		 * @param columns      Sorted column data from the column pass.
		 */
		void RenderWalls(int horizon, int screenHeight, const std::vector<ColumnInfo>& columns);

		/**
		 * @brief Draws a single solid-colour column using the column mesh.
		 *
		 * Used as a fallback when no wall sprite is assigned for a tile type.
		 *
		 * @param x      Screen-space X position (pixels from left).
		 * @param y      Screen-space Y top of the column (pixels from top).
		 * @param h      Column height in pixels.
		 * @param color  RGBA colour to render.
		 * @param sw     Screen width.
		 * @param sh     Screen height.
		 */
		void DrawColumn(f32 x, f32 y, f32 h, const Color& color, int sw, int sh);
	};

} // namespace Engine
