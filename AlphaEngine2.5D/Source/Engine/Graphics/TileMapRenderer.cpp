/**
 * @file TileMapRenderer.cpp
 * @brief Implementation of TileMapRenderer class
 */
#include "TileMapRenderer.hpp"
#include "ZBuffer.hpp"
#include "Engine/Core/Application.hpp"
#include <cmath>
#include <vector>
#include <future>
#include <algorithm>

namespace Engine {

	TileMapRenderer::TileMapRenderer() : columnMesh(nullptr) {}

	TileMapRenderer::~TileMapRenderer() {
		if (columnMesh) {
			AEGfxMeshFree(columnMesh);
		}
	}

	void TileMapRenderer::Render() {
		if (!enabled) return;

		TileMap* map = gameObject->GetComponent<TileMap>();
		if (!map) return;

		if (!columnMesh) {
			CreateColumnMesh();
		}

		// Reset AE camera to origin for screen-space rendering
		AEGfxSetCamPosition(0.0f, 0.0f);

		int screenWidth = AEGfxGetWindowWidth();
		int screenHeight = AEGfxGetWindowHeight();

		ZBuffer::Resize(screenWidth);
		ZBuffer::Clear();

		Transform* camTransform = (Camera::main) ? Camera::main->transform : transform;
		if (!camTransform) return;

		Vector2 pos = Vector2(camTransform->position.x, camTransform->position.y);
		f32 camZ = camTransform->position.z;
		f32 angleRad = camTransform->rotation * 3.14159f / 180.0f;
		f32 verticalOffset = (Camera::main) ? Camera::main->pitch : 0.0f;
		int horizon = screenHeight / 2 + (int)verticalOffset;

		f32 dirX = cosf(angleRad);
		f32 dirY = sinf(angleRad);
		f32 planeLength = tanf((fov * 3.14159f / 180.0f) / 2.0f);
		cachedPlaneLength = planeLength;
		f32 planeX = dirY * planeLength;
		f32 planeY = -dirX * planeLength;

		// Cache fog settings from Camera::main once per frame (#3)
		fog.enabled = (Camera::main && Camera::main->fogEnabled && !ignoreFog);
		fog.distance = (Camera::main) ? Camera::main->fogDistance : 20.0f;
		fog.minVisibility = (Camera::main) ? Camera::main->fogMinVisibility : 0.2f;
		fog.maxVisibility = (Camera::main) ? Camera::main->fogMaxVisibility : 1.0f;
		fog.precision = (Camera::main) ? Camera::main->fogPrecision : 200.0f;

		// Pre-compute identity matrix once per frame (#6)
		AEMtx33Identity(&identityMtx);

		// Update Fog LUT if distance changed
		f32 currentFogDist = fog.distance;
		if (fabsf(currentFogDist - cachedFogDistance) > 0.001f) {
			UpdateFogLUT(currentFogDist);
		}

		// Perform unified raycasting pass
		managedColumns.clear();
		int numCols = (screenWidth + COLUMN_STEP - 1) / COLUMN_STEP;
		managedColumns.resize(numCols);

		// Lambda for processing a range of columns
		auto ProcessColumnRange = [&](int startIndex, int endIndex) {
			for (int i = startIndex; i < endIndex; ++i) {
				int x = i * COLUMN_STEP;
				// Safety check for last column if screenWidth isn't multiple of COLUMN_STEP
				if (x >= screenWidth) break;

				f32 cameraX = 2.0f * x / (f32)screenWidth - 1.0f;
				f32 cameraX2 = 2.0f * (x + COLUMN_STEP) / (f32)screenWidth - 1.0f;

				ColumnInfo info;
				info.rayDirX = dirX + planeX * cameraX;
				info.rayDirY = dirY + planeY * cameraX;
				info.rayDirX2 = dirX + planeX * cameraX2;
				info.rayDirY2 = dirY + planeY * cameraX2;

				info.ray = CastRay(pos, info.rayDirX, info.rayDirY, camZ, horizon, screenHeight, map);

				f32 drawStart = info.ray.drawStart < 0 ? 0 : info.ray.drawStart;
				f32 drawEnd = info.ray.drawEnd >= screenHeight ? (f32)screenHeight - 1 : info.ray.drawEnd;

				// Fog LUT lookup (0.01 unit precision)
				f32 dist = info.ray.perpWallDist;
				int lutIdx = (int)(dist * 100.0f);
				if (lutIdx < 0) lutIdx = 0;
				if (lutIdx >= (int)fogLUT.size()) lutIdx = (int)fogLUT.size() - 1;

				info.fogFactor = fog.enabled ? fogLUT[lutIdx] : 1.0f;
				if (info.fogFactor < fog.minVisibility) info.fogFactor = fog.minVisibility;
				if (info.fogFactor > fog.maxVisibility) info.fogFactor = fog.maxVisibility;

				info.fogKey = (int)(info.fogFactor * fog.precision);

				info.screenX = (f32)x - screenWidth / 2.0f;
				info.screenX2 = (f32)(x + COLUMN_STEP) - screenWidth / 2.0f;
				if (info.screenX2 > screenWidth / 2.0f) info.screenX2 = screenWidth / 2.0f;

				info.screenY = screenHeight / 2.0f - drawEnd;
				info.screenY2 = screenHeight / 2.0f - drawStart;

				f32 unclippedHeight = info.ray.drawEnd - info.ray.drawStart;
				info.vTop = (drawStart - info.ray.drawStart) / unclippedHeight;
				info.vBottom = (drawEnd - info.ray.drawStart) / unclippedHeight;

				managedColumns[i] = info;

				// Fill Z-buffer (thread-safe as indices are distinct per column block)
				for (int k = 0; k < COLUMN_STEP && (x + k) < screenWidth; ++k) {
					ZBuffer::SetDepth(x + k, info.ray.perpWallDist);
				}
			}
		};

		// Async launch
		int numThreads = std::thread::hardware_concurrency();
		if (numThreads == 0) numThreads = 2; // Fallback
		
		int colsPerThread = (numCols + numThreads - 1) / numThreads;
		std::vector<std::future<void>> futures;

		for (int t = 0; t < numThreads; ++t) {
			int startIdx = t * colsPerThread;
			int endIdx = startIdx + colsPerThread;
			if (endIdx > numCols) endIdx = numCols;
			
			if (startIdx < endIdx) {
				futures.push_back(std::async(std::launch::async, ProcessColumnRange, startIdx, endIdx));
			}
		}

		// Wait for all
		for (auto& f : futures) {
			f.wait();
		}

		// Ensure linear filtering for floor/ceiling
		AEGfxTextureMode(AE_GFX_TM_AVERAGE);

		// Render in order: ceiling, floor, walls (walls on top)
		RenderCeiling(pos, camZ, horizon, screenWidth, screenHeight, managedColumns, map);
		RenderFloor(pos, camZ, horizon, screenWidth, screenHeight, managedColumns, map);

		// Sort walls for efficient batching: TileType first, then FogKey
		std::sort(managedColumns.begin(), managedColumns.end(), [](const ColumnInfo& a, const ColumnInfo& b) {
			if (a.ray.tileType != b.ray.tileType) return a.ray.tileType < b.ray.tileType;
			return a.fogKey < b.fogKey;
		});

		RenderWalls(horizon, screenHeight, managedColumns);
	}

	TileMapRenderer::RaycastResult TileMapRenderer::CastRay(const Vector2& pos, f32 rayDirX, f32 rayDirY, 
		f32 camZ, int horizon, int screenHeight, TileMap* map) {
		
		RaycastResult result;
		
		// Convert world position to tile grid coordinates
		int mapX = (int)floorf(pos.x);
		int mapY = (int)floorf(pos.y);
		f32 deltaDistX = (rayDirX == 0) ? 1e30f : fabsf(1.0f / rayDirX);
		f32 deltaDistY = (rayDirY == 0) ? 1e30f : fabsf(1.0f / rayDirY);
		f32 sideDistX, sideDistY;
		int stepX, stepY;

		if (rayDirX < 0) { stepX = -1; sideDistX = (pos.x - mapX) * deltaDistX; }
		else { stepX = 1; sideDistX = (mapX + 1.0f - pos.x) * deltaDistX; }
		if (rayDirY < 0) { stepY = -1; sideDistY = (pos.y - mapY) * deltaDistY; }
		else { stepY = 1; sideDistY = (mapY + 1.0f - pos.y) * deltaDistY; }

		result.side = 0;
		while (!map->IsWall(mapX, mapY)) {
			if (sideDistX < sideDistY) { sideDistX += deltaDistX; mapX += stepX; result.side = 0; }
			else { sideDistY += deltaDistY; mapY += stepY; result.side = 1; }
		}

		// Store the tile type that was hit
		result.tileType = static_cast<int>(map->GetTile(mapX, mapY));

		result.perpWallDist = (result.side == 0) ? (sideDistX - deltaDistX) : (sideDistY - deltaDistY);
		if (result.perpWallDist < 0.1f) result.perpWallDist = 0.1f;

		// Calculate where on the wall the ray hit (for texture mapping)
		if (result.side == 0) {
			result.wallX = pos.y + result.perpWallDist * rayDirY;
		} else {
			result.wallX = pos.x + result.perpWallDist * rayDirX;
		}
		// result.wallX is now the absolute world coordinate (for U mapping)

		// Wall height with aspect ratio correction
		int screenWidth = Application::GetWindowWidth();
		f32 wallHeight = (f32)screenWidth / (result.perpWallDist * cachedPlaneLength * 2.0f);
		f32 heightOffset = (camZ - 0.5f) * ((f32)screenWidth / (result.perpWallDist * cachedPlaneLength * 2.0f));
		result.drawStart = (f32)horizon - (wallHeight / 2.0f) + heightOffset;
		result.drawEnd = (f32)horizon + (wallHeight / 2.0f) + heightOffset;

		return result;
	}

	void TileMapRenderer::RenderCeiling(const Vector2& pos, f32 camZ, int horizon, int screenWidth, int screenHeight, 
		const std::vector<ColumnInfo>& columns, TileMap* map) {
		
		if (ceilingSprites.empty() || columns.empty()) return;

		auto sprite = GetCeilingSprite(1);
		if (!sprite || !sprite->IsLoaded()) return;

		f32 posZ = 1.0f - camZ;
		// Pre-compute row distance factor: rowDist = rowDistFactor / p (#4)
		f32 rowDistFactor = posZ * (f32)screenWidth / (cachedPlaneLength * 2.0f);

		f32 currentBatchFog = -1.0f;
		bool meshIsOpen = false;
		int triCount = 0;

		// Clamp horizon to screen bounds for culling
		int clampedHorizon = horizon;
		if (clampedHorizon < 0) clampedHorizon = 0;
		if (clampedHorizon > screenHeight) clampedHorizon = screenHeight;

		for (int y = 0; y < clampedHorizon; y += ROW_STEP) {
			f32 p = (f32)horizon - (f32)y;
			if (fabsf(p) < 0.1f) continue;
			f32 rowDist = rowDistFactor / p;
			if (rowDist < 0) continue;

			int yNext = y + ROW_STEP;
			if (yNext > horizon) yNext = horizon;
			if (yNext > clampedHorizon) yNext = clampedHorizon;

			f32 pNext = (f32)horizon - (f32)yNext;
			f32 rowDistNext;
			if (fabsf(pNext) < 0.1f) rowDistNext = rowDist;
			else rowDistNext = rowDistFactor / pNext;
			if (rowDistNext < 0) rowDistNext = rowDist;

			f32 aeY1 = screenHeight / 2.0f - (f32)yNext;
			f32 aeY2 = screenHeight / 2.0f - (f32)y;

			f32 avgDist = (rowDist + rowDistNext) * 0.5f;
			
			// Fog LUT lookup
			f32 fogFactor = 1.0f;
			if (fog.enabled) {
				int lutIdx = (int)(avgDist * 100.0f);
				if (lutIdx < 0) lutIdx = 0;
				if (lutIdx >= (int)fogLUT.size()) lutIdx = (int)fogLUT.size() - 1;
				fogFactor = fogLUT[lutIdx];
				if (fogFactor < fog.minVisibility) fogFactor = fog.minVisibility;
				if (fogFactor > fog.maxVisibility) fogFactor = fog.maxVisibility;
			}

			if (fogFactor < 0.01f) break;

			int fogKey = (int)(fogFactor * fog.precision);
			f32 quantizedFog = (f32)fogKey / fog.precision;

			// If fog changed, flush the current mesh
			if (meshIsOpen && quantizedFog != currentBatchFog) {
				AEGfxVertexList* mesh = AEGfxMeshEnd();
				if (mesh) {
					if (triCount > 0) {
						AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
						AEGfxTextureSet(sprite->GetTexture(), 0, 0);
						AEGfxSetBlendMode(AE_GFX_BM_BLEND);
						AEGfxSetColorToMultiply(currentBatchFog, currentBatchFog, currentBatchFog, 1.0f);
						AEGfxSetColorToAdd(0, 0, 0, 0);
						AEGfxSetTransparency(1.0f);
						AEGfxSetTransform(identityMtx.m);
						AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
					}
					AEGfxMeshFree(mesh);
				}
				meshIsOpen = false;
			}

			if (!meshIsOpen) {
				AEGfxMeshStart();
				meshIsOpen = true;
				triCount = 0;
				currentBatchFog = quantizedFog;
			}

			for (const auto& col : columns) {
				// Flush mesh if triangle limit reached
				if (triCount + 2 > MAX_TRIANGLES_PER_MESH) {
					AEGfxVertexList* mesh = AEGfxMeshEnd();
					if (mesh) {
						AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
						AEGfxTextureSet(sprite->GetTexture(), 0, 0);
						AEGfxSetBlendMode(AE_GFX_BM_BLEND);
						AEGfxSetColorToMultiply(currentBatchFog, currentBatchFog, currentBatchFog, 1.0f);
						AEGfxSetColorToAdd(0, 0, 0, 0);
						AEGfxSetTransparency(1.0f);
						AEGfxSetTransform(identityMtx.m);
						AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
						AEGfxMeshFree(mesh);
					}
					AEGfxMeshStart();
					triCount = 0;
				}

				f32 clampedAeY1 = aeY1;
				f32 clampedRowDistNext = rowDistNext;
				
				int wallStartInt = (int)floorf(col.ray.drawStart);
				if (yNext > wallStartInt) {
					if (y >= wallStartInt) continue;
					clampedAeY1 = col.screenY2;
					f32 clampedP = (f32)horizon - col.ray.drawStart;
					if (fabsf(clampedP) > 0.1f) {
						clampedRowDistNext = rowDistFactor / clampedP;
					}
				}

				f32 u1_l = pos.x + rowDist * col.rayDirX;
				f32 v1_l = pos.y + rowDist * col.rayDirY;
				f32 u1_r = pos.x + rowDist * col.rayDirX2;
				f32 v1_r = pos.y + rowDist * col.rayDirY2;

				f32 u2_l = pos.x + clampedRowDistNext * col.rayDirX;
				f32 v2_l = pos.y + clampedRowDistNext * col.rayDirY;
				f32 u2_r = pos.x + clampedRowDistNext * col.rayDirX2;
				f32 v2_r = pos.y + clampedRowDistNext * col.rayDirY2;

				AEGfxTriAdd(col.screenX, aeY2, 0xFFFFFFFF, u1_l, v1_l, 
				            col.screenX2, aeY2, 0xFFFFFFFF, u1_r, v1_r, 
				            col.screenX, clampedAeY1, 0xFFFFFFFF, u2_l, v2_l);
				AEGfxTriAdd(col.screenX, clampedAeY1, 0xFFFFFFFF, u2_l, v2_l, 
				            col.screenX2, aeY2, 0xFFFFFFFF, u1_r, v1_r, 
				            col.screenX2, clampedAeY1, 0xFFFFFFFF, u2_r, v2_r);
				triCount += 2;
			}
		}

		if (meshIsOpen) {
			AEGfxVertexList* mesh = AEGfxMeshEnd();
			if (mesh) {
				if (triCount > 0) {
					AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
					AEGfxTextureSet(sprite->GetTexture(), 0, 0);
					AEGfxSetBlendMode(AE_GFX_BM_BLEND);
					AEGfxSetColorToMultiply(currentBatchFog, currentBatchFog, currentBatchFog, 1.0f);
					AEGfxSetColorToAdd(0, 0, 0, 0);
					AEGfxSetTransparency(1.0f);
					AEGfxSetTransform(identityMtx.m);
					AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
				}
				AEGfxMeshFree(mesh);
			}
		}
	}
	void TileMapRenderer::RenderFloor(const Vector2& pos, f32 camZ, int horizon, int screenWidth, int screenHeight, 
		const std::vector<ColumnInfo>& columns, TileMap* map) {
		
		if (floorSprites.empty() || columns.empty()) return;

		auto sprite = GetFloorSprite(1);
		if (!sprite || !sprite->IsLoaded()) return;

		f32 posZ = camZ;
		// Pre-compute row distance factor: rowDist = rowDistFactor / p (#4)
		f32 rowDistFactor = posZ * (f32)screenWidth / (cachedPlaneLength * 2.0f);

		f32 currentBatchFog = -1.0f;
		bool meshIsOpen = false;
		int triCount = 0;

		// Clamp horizon to screen bounds for culling
		int clampedHorizon = horizon;
		if (clampedHorizon < 0) clampedHorizon = 0;
		if (clampedHorizon > screenHeight) clampedHorizon = screenHeight;

		// Skip invisible far rows near horizon (#3 optimization)
		int startY = clampedHorizon;
		if (fog.enabled && posZ > 0.001f) {
			f32 maxVisibleDist = fog.distance * 4.6f;
			f32 minP = rowDistFactor / maxVisibleDist;
			int computedStart = horizon + (int)ceilf(minP);
			if (computedStart > clampedHorizon) {
				startY = ((computedStart - clampedHorizon) / ROW_STEP) * ROW_STEP + clampedHorizon;
			}
		}

		for (int y = startY; y < screenHeight; y += ROW_STEP) {
			f32 p = (f32)y - (f32)horizon;
			if (fabsf(p) < 0.1f) continue;
			f32 rowDist = rowDistFactor / p;
			if (rowDist < 0) continue;

			int yNext = y + ROW_STEP;
			if (yNext > screenHeight) yNext = screenHeight;

			f32 pNext = (f32)yNext - (f32)horizon;
			f32 rowDistNext;
			if (fabsf(pNext) < 0.1f) rowDistNext = rowDist;
			else rowDistNext = rowDistFactor / pNext;
			if (rowDistNext < 0) rowDistNext = rowDist;

			f32 aeY1 = screenHeight / 2.0f - (f32)yNext;
			f32 aeY2 = screenHeight / 2.0f - (f32)y;

			f32 avgDist = (rowDist + rowDistNext) * 0.5f;

			// Fog LUT lookup
			f32 fogFactor = 1.0f;
			if (fog.enabled) {
				int lutIdx = (int)(avgDist * 100.0f);
				if (lutIdx < 0) lutIdx = 0;
				if (lutIdx >= (int)fogLUT.size()) lutIdx = (int)fogLUT.size() - 1;
				fogFactor = fogLUT[lutIdx];
				if (fogFactor < fog.minVisibility) fogFactor = fog.minVisibility;
				if (fogFactor > fog.maxVisibility) fogFactor = fog.maxVisibility;
			}

			if (fogFactor < 0.01f) continue;

			int fogKey = (int)(fogFactor * fog.precision);
			f32 quantizedFog = (f32)fogKey / fog.precision;

			// If fog changed, flush the current mesh
			if (meshIsOpen && quantizedFog != currentBatchFog) {
				AEGfxVertexList* mesh = AEGfxMeshEnd();
				if (mesh) {
					if (triCount > 0) {
						AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
						AEGfxTextureSet(sprite->GetTexture(), 0, 0);
						AEGfxSetBlendMode(AE_GFX_BM_BLEND);
						AEGfxSetColorToMultiply(currentBatchFog, currentBatchFog, currentBatchFog, 1.0f);
						AEGfxSetColorToAdd(0, 0, 0, 0);
						AEGfxSetTransparency(1.0f);
						AEGfxSetTransform(identityMtx.m);
						AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
					}
					AEGfxMeshFree(mesh);
				}
				meshIsOpen = false;
			}

			if (!meshIsOpen) {
				AEGfxMeshStart();
				meshIsOpen = true;
				triCount = 0;
				currentBatchFog = quantizedFog;
			}

			for (const auto& col : columns) {
				// Flush mesh if triangle limit reached
				if (triCount + 2 > MAX_TRIANGLES_PER_MESH) {
					AEGfxVertexList* mesh = AEGfxMeshEnd();
					if (mesh) {
						AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
						AEGfxTextureSet(sprite->GetTexture(), 0, 0);
						AEGfxSetBlendMode(AE_GFX_BM_BLEND);
						AEGfxSetColorToMultiply(currentBatchFog, currentBatchFog, currentBatchFog, 1.0f);
						AEGfxSetColorToAdd(0, 0, 0, 0);
						AEGfxSetTransparency(1.0f);
						AEGfxSetTransform(identityMtx.m);
						AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
						AEGfxMeshFree(mesh);
					}
					AEGfxMeshStart();
					triCount = 0;
				}

				f32 clampedAeY2 = aeY2;
				f32 clampedRowDist = rowDist;
				
				int wallEndInt = (int)ceilf(col.ray.drawEnd);
				if (y < wallEndInt) {
					if (yNext <= wallEndInt) continue;
					clampedAeY2 = col.screenY;
					f32 clampedP = col.ray.drawEnd - (f32)horizon;
					if (fabsf(clampedP) > 0.1f) {
						clampedRowDist = rowDistFactor / clampedP;
					}
				}

				f32 u1_l = pos.x + clampedRowDist * col.rayDirX;
				f32 v1_l = pos.y + clampedRowDist * col.rayDirY;
				f32 u1_r = pos.x + clampedRowDist * col.rayDirX2;
				f32 v1_r = pos.y + clampedRowDist * col.rayDirY2;

				f32 u2_l = pos.x + rowDistNext * col.rayDirX;
				f32 v2_l = pos.y + rowDistNext * col.rayDirY;
				f32 u2_r = pos.x + rowDistNext * col.rayDirX2;
				f32 v2_r = pos.y + rowDistNext * col.rayDirY2;

				AEGfxTriAdd(col.screenX, clampedAeY2, 0xFFFFFFFF, u1_l, v1_l, 
				            col.screenX2, clampedAeY2, 0xFFFFFFFF, u1_r, v1_r, 
				            col.screenX, aeY1, 0xFFFFFFFF, u2_l, v2_l);
				AEGfxTriAdd(col.screenX, aeY1, 0xFFFFFFFF, u2_l, v2_l, 
				            col.screenX2, clampedAeY2, 0xFFFFFFFF, u1_r, v1_r, 
				            col.screenX2, aeY1, 0xFFFFFFFF, u2_r, v2_r);
				triCount += 2;
			}
		}

		if (meshIsOpen) {
			AEGfxVertexList* mesh = AEGfxMeshEnd();
			if (mesh) {
				if (triCount > 0) {
					AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
					AEGfxTextureSet(sprite->GetTexture(), 0, 0);
					AEGfxSetBlendMode(AE_GFX_BM_BLEND);
					AEGfxSetColorToMultiply(currentBatchFog, currentBatchFog, currentBatchFog, 1.0f);
					AEGfxSetColorToAdd(0, 0, 0, 0);
					AEGfxSetTransparency(1.0f);
					AEGfxSetTransform(identityMtx.m);
					AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
				}
				AEGfxMeshFree(mesh);
			}
		}
	}

	void TileMapRenderer::RenderWalls(int horizon, int screenHeight, const std::vector<ColumnInfo>& columns) {
		if (columns.empty()) return;

		AEGfxTextureMode(AE_GFX_TM_AVERAGE);

		// Since columns are already sorted by tileType and fogKey, we can batch linearly
		int currentTileType = -1;
		int currentFogKey = -1;
		bool meshIsOpen = false;
		int triCount = 0;
		Sprite* currentSprite = nullptr;

		Color cNS = wallColorNS;
		Color cEW = wallColorEW;

		for (const auto& col : columns) {
			// Vertical Culling: skip if entirely above/below or zero height
			if (col.ray.drawEnd < 0 || col.ray.drawStart > screenHeight || col.screenY == col.screenY2) continue;

			// Fog Culling
			if (col.fogFactor < 0.01f) continue;

			// If type or fog changed, flush
			if (meshIsOpen && (col.ray.tileType != currentTileType || col.fogKey != currentFogKey)) {
				AEGfxVertexList* mesh = AEGfxMeshEnd();
				if (mesh) {
					if (currentSprite) {
						AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
						AEGfxTextureSet(currentSprite->GetTexture(), 0, 0);
						AEGfxSetBlendMode(AE_GFX_BM_BLEND);
						float drawFog = (float)currentFogKey / fog.precision;
						AEGfxSetColorToMultiply(drawFog, drawFog, drawFog, 1.0f);
						AEGfxSetColorToAdd(0, 0, 0, 0);
						AEGfxSetTransparency(1.0f);
					} else {
						AEGfxSetRenderMode(AE_GFX_RM_COLOR);
						AEGfxSetBlendMode(AE_GFX_BM_NONE);
						AEGfxSetColorToMultiply(1, 1, 1, 1);
						AEGfxSetColorToAdd(0, 0, 0, 0);
					}
					AEGfxSetTransform(identityMtx.m);
					AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
					AEGfxMeshFree(mesh);
				}
				meshIsOpen = false;
			}

			if (!meshIsOpen) {
				AEGfxMeshStart();
				meshIsOpen = true;
				triCount = 0;
				currentTileType = col.ray.tileType;
				currentFogKey = col.fogKey;
				currentSprite = GetWallSprite(currentTileType);
			}

			if (currentSprite) {
				AEGfxTriAdd(col.screenX, col.screenY2, 0xFFFFFFFF, col.ray.wallX, col.vTop,
							col.screenX2, col.screenY2, 0xFFFFFFFF, col.ray.wallX, col.vTop,
							col.screenX, col.screenY, 0xFFFFFFFF, col.ray.wallX, col.vBottom);
				AEGfxTriAdd(col.screenX, col.screenY, 0xFFFFFFFF, col.ray.wallX, col.vBottom,
							col.screenX2, col.screenY2, 0xFFFFFFFF, col.ray.wallX, col.vTop,
							col.screenX2, col.screenY, 0xFFFFFFFF, col.ray.wallX, col.vBottom);
			} else {
				Color c = (col.ray.side == 0) ? cNS : cEW;
				float f = (float)currentFogKey / fog.precision;
				u32 wallColorU32 = (((u32)(c.r * f * 255)) << 24) | (((u32)(c.g * f * 255)) << 16) | (((u32)(c.b * f * 255)) << 8) | 0xFF;
				AEGfxTriAdd(col.screenX, col.screenY2, wallColorU32, 0, 0, col.screenX2, col.screenY2, wallColorU32, 0, 0, col.screenX, col.screenY, wallColorU32, 0, 0);
				AEGfxTriAdd(col.screenX, col.screenY, wallColorU32, 0, 0, col.screenX2, col.screenY2, wallColorU32, 0, 0, col.screenX2, col.screenY, wallColorU32, 0, 0);
			}
			triCount += 2;

			if (triCount >= MAX_TRIANGLES_PER_MESH) {
				AEGfxVertexList* mesh = AEGfxMeshEnd();
				if (mesh) {
					if (currentSprite) {
						AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
						AEGfxTextureSet(currentSprite->GetTexture(), 0, 0);
						AEGfxSetBlendMode(AE_GFX_BM_BLEND);
						float drawFog = (float)currentFogKey / fog.precision;
						AEGfxSetColorToMultiply(drawFog, drawFog, drawFog, 1.0f);
						AEGfxSetColorToAdd(0, 0, 0, 0);
						AEGfxSetTransparency(1.0f);
					} else {
						AEGfxSetRenderMode(AE_GFX_RM_COLOR);
						AEGfxSetBlendMode(AE_GFX_BM_NONE);
						AEGfxSetColorToMultiply(1, 1, 1, 1);
						AEGfxSetColorToAdd(0, 0, 0, 0);
					}
					AEGfxSetTransform(identityMtx.m);
					AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
					AEGfxMeshFree(mesh);
				}
				AEGfxMeshStart();
				triCount = 0;
			}
		}

		if (meshIsOpen) {
			AEGfxVertexList* mesh = AEGfxMeshEnd();
			if (mesh) {
				if (currentSprite) {
					AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
					AEGfxTextureSet(currentSprite->GetTexture(), 0, 0);
					AEGfxSetBlendMode(AE_GFX_BM_BLEND);
					float drawFog = (float)currentFogKey / fog.precision;
					AEGfxSetColorToMultiply(drawFog, drawFog, drawFog, 1.0f);
					AEGfxSetColorToAdd(0, 0, 0, 0);
					AEGfxSetTransparency(1.0f);
				} else {
					AEGfxSetRenderMode(AE_GFX_RM_COLOR);
					AEGfxSetBlendMode(AE_GFX_BM_NONE);
					AEGfxSetColorToMultiply(1, 1, 1, 1);
					AEGfxSetColorToAdd(0, 0, 0, 0);
				}
				AEGfxSetTransform(identityMtx.m);
				AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
				AEGfxMeshFree(mesh);
			}
		}
	}

	void TileMapRenderer::CreateColumnMesh() {
		AEGfxMeshStart();
		AEGfxTriAdd(0.0f, 0.0f, 0xFFFFFFFF, 0.0f, 1.0f, 0.0f, 1.0f, 0xFFFFFFFF, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 1.0f, 0.0f);
		AEGfxTriAdd(0.0f, 0.0f, 0xFFFFFFFF, 0.0f, 1.0f, 1.0f, 1.0f, 0xFFFFFFFF, 1.0f, 0.0f, 1.0f, 0.0f, 0xFFFFFFFF, 1.0f, 1.0f);
		columnMesh = AEGfxMeshEnd();
	}

	void TileMapRenderer::DrawColumn(f32 x, f32 y, f32 h, const Color& color, int sw, int sh) {
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		AEGfxSetBlendMode(AE_GFX_BM_NONE);
		AEGfxSetColorToMultiply(color.r, color.g, color.b, color.a);
		AEGfxSetColorToAdd(0, 0, 0, 0);

		AEMtx33 scale, trans, result;
		AEMtx33Scale(&scale, 1.0f, h);
		f32 screenX = x - sw / 2.0f;
		f32 screenY = sh / 2.0f - y - h;
		AEMtx33Trans(&trans, screenX, screenY);
		AEMtx33Concat(&result, &trans, &scale);
		AEGfxSetTransform(result.m);
		AEGfxMeshDraw(columnMesh, AE_GFX_MDM_TRIANGLES);
	}


	void TileMapRenderer::UpdateFogLUT(f32 fogDistance) {
		fogLUT.resize(10000); // Up to distance 100 with 0.01 precision
		for (size_t i = 0; i < fogLUT.size(); ++i) {
			f32 dist = (f32)i * 0.01f;
			fogLUT[i] = expf(-dist / fogDistance);
		}
		cachedFogDistance = fogDistance;
	}

} // namespace Engine

