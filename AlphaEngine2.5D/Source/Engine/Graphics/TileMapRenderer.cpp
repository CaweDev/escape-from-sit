/**
 * @file TileMapRenderer.cpp
 * @brief Implementation of TileMapRenderer class
 */
#include "TileMapRenderer.hpp"
#include "ZBuffer.hpp"
#include <cmath>
#include <vector>

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
		f32 planeX = -dirY * planeLength;
		f32 planeY = dirX * planeLength;

		// Perform unified raycasting pass
		std::vector<ColumnInfo> columns;
		columns.reserve(screenWidth / COLUMN_STEP + 1);

		for (int x = 0; x < screenWidth; x += COLUMN_STEP) {
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

			info.fogFactor = 1.0f - (info.ray.perpWallDist / 20.0f);
			if (info.fogFactor < 0.2f) info.fogFactor = 0.2f;

			info.screenX = (f32)x - screenWidth / 2.0f;
			info.screenX2 = (f32)(x + COLUMN_STEP) - screenWidth / 2.0f;
			if (info.screenX2 > screenWidth / 2.0f) info.screenX2 = screenWidth / 2.0f;

			info.screenY = screenHeight / 2.0f - drawEnd;
			info.screenY2 = screenHeight / 2.0f - drawStart;

			f32 unclippedHeight = info.ray.drawEnd - info.ray.drawStart;
			info.vTop = (drawStart - info.ray.drawStart) / unclippedHeight;
			info.vBottom = (drawEnd - info.ray.drawStart) / unclippedHeight;

			columns.push_back(info);

			// Fill Z-buffer
			for (int i = 0; i < COLUMN_STEP && (x + i) < screenWidth; ++i) {
				ZBuffer::SetDepth(x + i, info.ray.perpWallDist);
			}
		}

		// Pre-cache scanline distances (only recomputes if screenHeight or camZ or horizon changed)
		CacheScanlineDistances(screenHeight, camZ, horizon);

		// Render in order: ceiling, floor, walls (walls on top)
		RenderCeiling(pos, camZ, horizon, screenWidth, screenHeight, columns, map);
		RenderFloor(pos, camZ, horizon, screenWidth, screenHeight, columns, map);
		RenderWalls(horizon, screenHeight, columns);
	}

	TileMapRenderer::RaycastResult TileMapRenderer::CastRay(const Vector2& pos, f32 rayDirX, f32 rayDirY, 
		f32 camZ, int horizon, int screenHeight, TileMap* map) {
		
		RaycastResult result;
		
		int mapX = (int)pos.x;
		int mapY = (int)pos.y;
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

		f32 wallHeight = (f32)screenHeight / result.perpWallDist;
		f32 heightOffset = (camZ - 0.5f) * ((f32)screenHeight / result.perpWallDist);
		result.drawStart = (f32)horizon - (wallHeight / 2.0f) + heightOffset;
		result.drawEnd = (f32)horizon + (wallHeight / 2.0f) + heightOffset;

		return result;
	}

	void TileMapRenderer::RenderCeiling(const Vector2& pos, f32 camZ, int horizon, int screenWidth, int screenHeight, 
		const std::vector<ColumnInfo>& columns, TileMap* map) {
		
		// Check if we have any ceiling sprites
		if (ceilingSprites.empty()) return;
		if (columns.empty()) return;

		auto sprite = GetCeilingSprite(1);
		if (!sprite || !sprite->IsLoaded()) return;

		// Ceiling height is 1.0 - camZ above the viewer
		f32 posZ = 1.0f - camZ;

		int triCount = 0;
		bool meshIsOpen = false;

		// Scanline-based rendering: use original loop structure with cached rowDist lookup
		for (int y = 0; y < horizon; y += ROW_STEP) {
			int scanIdx = y / ROW_STEP;
			if (scanIdx >= (int)ceilingScanlines.size()) continue;
			
			f32 rowDist = ceilingScanlines[scanIdx].rowDist;
			if (rowDist < 0) continue; // Invalid scanline

			int yNext = y + ROW_STEP;
			if (yNext > horizon) yNext = horizon;
			
			int scanIdxNext = yNext / ROW_STEP;
			f32 rowDistNext = (scanIdxNext < (int)ceilingScanlines.size()) 
				? ceilingScanlines[scanIdxNext].rowDist 
				: rowDist;
			if (rowDistNext < 0) rowDistNext = rowDist;

			f32 aeY1 = screenHeight / 2.0f - (f32)yNext;
			f32 aeY2 = screenHeight / 2.0f - (f32)y;

			// Now iterate columns for this scanline
			for (const auto& col : columns) {
				// Calculate clamped Y coordinates for this column
				// This prevents gaps at wall-ceiling boundary
				f32 clampedAeY1 = aeY1;
				f32 clampedRowDistNext = rowDistNext;
				
				// If this scanline band's bottom is below wall start, clamp to wall start
				if (yNext > (int)col.ray.drawStart) {
					// Skip entirely if even the top of this band is below wall start
					if (y >= (int)col.ray.drawStart) continue;
					
					// Clamp bottom of quad to wall boundary
					clampedAeY1 = screenHeight / 2.0f - col.ray.drawStart;
					f32 clampedP = (f32)horizon - col.ray.drawStart;
					if (fabsf(clampedP) > 0.1f) {
						clampedRowDistNext = posZ * (f32)screenHeight / clampedP;
					}
				}

				if (!meshIsOpen) {
					AEGfxMeshStart();
					meshIsOpen = true;
					triCount = 0;
				}

				// UV computed using shared rowDist (interpolated across both sides)
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

				if (triCount >= MAX_TRIANGLES_PER_MESH) {
					AEGfxVertexList* mesh = AEGfxMeshEnd();
					if (mesh) {
						AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
						AEGfxTextureSet(sprite->GetTexture(), 0, 0);
						AEGfxSetBlendMode(AE_GFX_BM_BLEND);
						AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
						AEGfxSetColorToAdd(0, 0, 0, 0);
						AEGfxSetTransparency(1.0f);
						AEMtx33 identity; AEMtx33Identity(&identity); AEGfxSetTransform(identity.m);
						AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
						AEGfxMeshFree(mesh);
					}
					meshIsOpen = false;
				}
			}
		}
		
		if (meshIsOpen) {
			AEGfxVertexList* mesh = AEGfxMeshEnd();
			if (mesh) {
				AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
				AEGfxTextureSet(sprite->GetTexture(), 0, 0);
				AEGfxSetBlendMode(AE_GFX_BM_BLEND);
				AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
				AEGfxSetColorToAdd(0, 0, 0, 0);
				AEGfxSetTransparency(1.0f);
				AEMtx33 identity; AEMtx33Identity(&identity); AEGfxSetTransform(identity.m);
				AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
				AEGfxMeshFree(mesh);
			}
		}
	}

	void TileMapRenderer::RenderFloor(const Vector2& pos, f32 camZ, int horizon, int screenWidth, int screenHeight, 
		const std::vector<ColumnInfo>& columns, TileMap* map) {
		
		// Check if we have any floor sprites
		if (floorSprites.empty()) return;
		if (columns.empty()) return;

		auto sprite = GetFloorSprite(1);
		if (!sprite || !sprite->IsLoaded()) return;

		// Pre-calculate row distances (done once, outside scanline loop)
		f32 posZ = camZ;

		int triCount = 0;
		bool meshIsOpen = false;

		// Scanline-based rendering: use original loop structure with cached rowDist lookup
		for (int y = horizon; y < screenHeight; y += ROW_STEP) {
			int scanIdx = (y - horizon) / ROW_STEP;
			if (scanIdx >= (int)floorScanlines.size()) continue;
			
			f32 rowDist = floorScanlines[scanIdx].rowDist;
			if (rowDist < 0) continue; // Invalid scanline

			int yNext = y + ROW_STEP;
			if (yNext > screenHeight) yNext = screenHeight;
			
			int scanIdxNext = (yNext - horizon) / ROW_STEP;
			f32 rowDistNext = (scanIdxNext < (int)floorScanlines.size()) 
				? floorScanlines[scanIdxNext].rowDist 
				: rowDist;
			if (rowDistNext < 0) rowDistNext = rowDist;

			f32 aeY1 = screenHeight / 2.0f - (f32)yNext;
			f32 aeY2 = screenHeight / 2.0f - (f32)y;

		// Now iterate columns for this scanline
			for (const auto& col : columns) {
				// Calculate clamped Y coordinates for this column
				// This prevents gaps at wall-floor boundary
				f32 clampedAeY2 = aeY2;
				f32 clampedRowDist = rowDist;
				
				// If this scanline row starts above the wall end, clamp to wall end
				if (y < (int)col.ray.drawEnd) {
					// Skip entirely if even the bottom of this band is above wall end
					if (yNext <= (int)col.ray.drawEnd) continue;
					
					// Clamp top of quad to wall boundary
					clampedAeY2 = screenHeight / 2.0f - col.ray.drawEnd;
					f32 clampedP = col.ray.drawEnd - (f32)horizon;
					if (fabsf(clampedP) > 0.1f) {
						clampedRowDist = posZ * (f32)screenHeight / clampedP;
					}
				}

				if (!meshIsOpen) {
					AEGfxMeshStart();
					meshIsOpen = true;
					triCount = 0;
				}

				// UV computed using shared rowDist (interpolated across both sides)
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

				if (triCount >= MAX_TRIANGLES_PER_MESH) {
					AEGfxVertexList* mesh = AEGfxMeshEnd();
					if (mesh) {
						AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
						AEGfxTextureSet(sprite->GetTexture(), 0, 0);
						AEGfxSetBlendMode(AE_GFX_BM_BLEND);
						AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
						AEGfxSetColorToAdd(0, 0, 0, 0);
						AEGfxSetTransparency(1.0f);
						AEMtx33 identity; AEMtx33Identity(&identity); AEGfxSetTransform(identity.m);
						AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
						AEGfxMeshFree(mesh);
					}
					meshIsOpen = false;
				}
			}
		}

		if (meshIsOpen) {
			AEGfxVertexList* mesh = AEGfxMeshEnd();
			if (mesh) {
				AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
				AEGfxTextureSet(sprite->GetTexture(), 0, 0);
				AEGfxSetBlendMode(AE_GFX_BM_BLEND);
				AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
				AEGfxSetColorToAdd(0, 0, 0, 0);
				AEGfxSetTransparency(1.0f);
				AEMtx33 identity; AEMtx33Identity(&identity); AEGfxSetTransform(identity.m);
				AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
				AEGfxMeshFree(mesh);
			}
		}
	}

	void TileMapRenderer::RenderWalls(int horizon, int screenHeight, const std::vector<ColumnInfo>& columns) {
		
		bool hasCeilingSprites = !ceilingSprites.empty();
		bool hasFloorSprites = !floorSprites.empty();
		AEGfxTextureMode(AE_GFX_TM_AVERAGE);

		// Sequential Batch 1: Ceiling Fallback
		if (!hasCeilingSprites) {
			u32 ceilColorU32 = (((u32)(ceilingColor.r * 255)) << 24) | (((u32)(ceilingColor.g * 255)) << 16) | (((u32)(ceilingColor.b * 255)) << 8) | (u32)(ceilingColor.a * 255);
			int triCount = 0;
			bool meshIsOpen = false;
			
			f32 screenTopY = (f32)screenHeight / 2.0f;
			for (const auto& col : columns) {
				f32 wallStart = col.screenY2;
				for (f32 y = screenTopY; y > wallStart; y -= ROW_STEP) {
					if (!meshIsOpen) {
						AEGfxMeshStart();
						meshIsOpen = true;
						triCount = 0;
					}

					f32 yNext = y - ROW_STEP;
					if (yNext < wallStart) yNext = wallStart;
					AEGfxTriAdd(col.screenX, y, ceilColorU32, 0, 0, col.screenX2, y, ceilColorU32, 0, 0, col.screenX, yNext, ceilColorU32, 0, 0);
					AEGfxTriAdd(col.screenX, yNext, ceilColorU32, 0, 0, col.screenX2, y, ceilColorU32, 0, 0, col.screenX2, yNext, ceilColorU32, 0, 0);
					triCount += 2;

					if (triCount >= MAX_TRIANGLES_PER_MESH) {
						AEGfxVertexList* mesh = AEGfxMeshEnd();
						if (mesh) {
							AEGfxSetRenderMode(AE_GFX_RM_COLOR);
							AEGfxSetBlendMode(AE_GFX_BM_NONE);
							AEGfxSetColorToMultiply(1, 1, 1, 1);
							AEGfxSetColorToAdd(0, 0, 0, 0);
							AEMtx33 identity; AEMtx33Identity(&identity); AEGfxSetTransform(identity.m);
							AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
							AEGfxMeshFree(mesh);
						}
						meshIsOpen = false;
					}
				}
			}

			if (meshIsOpen) {
				AEGfxVertexList* mesh = AEGfxMeshEnd();
				if (mesh) {
					AEGfxSetRenderMode(AE_GFX_RM_COLOR);
					AEGfxSetBlendMode(AE_GFX_BM_NONE);
					AEGfxSetColorToMultiply(1, 1, 1, 1);
					AEGfxSetColorToAdd(0, 0, 0, 0);
					AEMtx33 identity; AEMtx33Identity(&identity); AEGfxSetTransform(identity.m);
					AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
					AEGfxMeshFree(mesh);
				}
			}
		}

		// Sequential Batch 2: Floor Fallback
		if (!hasFloorSprites) {
			u32 floorColorU32 = (((u32)(floorColor.r * 255)) << 24) | (((u32)(floorColor.g * 255)) << 16) | (((u32)(floorColor.b * 255)) << 8) | (u32)(floorColor.a * 255);
			int triCount = 0;
			bool meshIsOpen = false;
			
			f32 screenBottomY = -(f32)screenHeight / 2.0f;
			for (const auto& col : columns) {
				f32 wallEnd = col.screenY;
				for (f32 y = wallEnd; y > screenBottomY; y -= ROW_STEP) {
					if (!meshIsOpen) {
						AEGfxMeshStart();
						meshIsOpen = true;
						triCount = 0;
					}

					f32 yNext = y - ROW_STEP;
					if (yNext < screenBottomY) yNext = screenBottomY;
					AEGfxTriAdd(col.screenX, y, floorColorU32, 0, 0, col.screenX2, y, floorColorU32, 0, 0, col.screenX, yNext, floorColorU32, 0, 0);
					AEGfxTriAdd(col.screenX, yNext, floorColorU32, 0, 0, col.screenX2, y, floorColorU32, 0, 0, col.screenX2, yNext, floorColorU32, 0, 0);
					triCount += 2;

					if (triCount >= MAX_TRIANGLES_PER_MESH) {
						AEGfxVertexList* mesh = AEGfxMeshEnd();
						if (mesh) {
							AEGfxSetRenderMode(AE_GFX_RM_COLOR);
							AEGfxSetBlendMode(AE_GFX_BM_NONE);
							AEGfxSetColorToMultiply(1, 1, 1, 1);
							AEGfxSetColorToAdd(0, 0, 0, 0);
							AEMtx33 identity; AEMtx33Identity(&identity); AEGfxSetTransform(identity.m);
							AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
							AEGfxMeshFree(mesh);
						}
						meshIsOpen = false;
					}
				}
			}

			if (meshIsOpen) {
				AEGfxVertexList* mesh = AEGfxMeshEnd();
				if (mesh) {
					AEGfxSetRenderMode(AE_GFX_RM_COLOR);
					AEGfxSetBlendMode(AE_GFX_BM_NONE);
					AEGfxSetColorToMultiply(1, 1, 1, 1);
					AEGfxSetColorToAdd(0, 0, 0, 0);
					AEMtx33 identity; AEMtx33Identity(&identity); AEGfxSetTransform(identity.m);
					AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
					AEGfxMeshFree(mesh);
				}
			}
		}

		// Sequential Batch 3: Walls (Textured by tile type, or Fallback)
		// Group columns by tile type for batching
		std::unordered_map<int, std::vector<const ColumnInfo*>> columnsByType;
		for (const auto& col : columns) {
			columnsByType[col.ray.tileType].push_back(&col);
		}

		// Render each tile type's walls
		for (const auto& pair : columnsByType) {
			int tileType = pair.first;
			const auto& typeCols = pair.second;
			
			auto sprite = GetWallSprite(tileType);
			bool hasSprite = sprite && sprite->IsLoaded();

			int triCount = 0;
			bool meshIsOpen = false;
			
			for (const ColumnInfo* colPtr : typeCols) {
				const ColumnInfo& col = *colPtr;
				
				if (hasSprite) {
					u32 fogColor = (((u32)(col.fogFactor * 255)) << 24) | (((u32)(col.fogFactor * 255)) << 16) | (((u32)(col.fogFactor * 255)) << 8) | 0xFF;
					
					if (!meshIsOpen) {
						AEGfxMeshStart();
						meshIsOpen = true;
						triCount = 0;
					}

					// Single quad per column - GPU handles texture interpolation
					AEGfxTriAdd(col.screenX, col.screenY2, fogColor, col.ray.wallX, col.vTop,
					            col.screenX2, col.screenY2, fogColor, col.ray.wallX, col.vTop,
					            col.screenX, col.screenY, fogColor, col.ray.wallX, col.vBottom);
					AEGfxTriAdd(col.screenX, col.screenY, fogColor, col.ray.wallX, col.vBottom,
					            col.screenX2, col.screenY2, fogColor, col.ray.wallX, col.vTop,
					            col.screenX2, col.screenY, fogColor, col.ray.wallX, col.vBottom);
					triCount += 2;

					if (triCount >= MAX_TRIANGLES_PER_MESH) {
						AEGfxVertexList* mesh = AEGfxMeshEnd();
						if (mesh) {
							AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
							AEGfxTextureSet(sprite->GetTexture(), 0, 0);
							AEGfxSetBlendMode(AE_GFX_BM_BLEND);
							AEGfxSetColorToMultiply(1, 1, 1, 1);
							AEGfxSetColorToAdd(0, 0, 0, 0);
							AEGfxSetTransparency(1.0f);
							AEMtx33 identity; AEMtx33Identity(&identity); AEGfxSetTransform(identity.m);
							AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
							AEGfxMeshFree(mesh);
						}
						meshIsOpen = false;
					}
				} else {
					Color c = (col.ray.side == 0) ? wallColorNS : wallColorEW;
					u32 wallColorU32 = (((u32)(c.r * col.fogFactor * 255)) << 24) | (((u32)(c.g * col.fogFactor * 255)) << 16) | (((u32)(c.b * col.fogFactor * 255)) << 8) | 0xFF;
					
					if (!meshIsOpen) {
						AEGfxMeshStart();
						meshIsOpen = true;
						triCount = 0;
					}

					// Single quad per column - no ROW_STEP subdivision needed
					AEGfxTriAdd(col.screenX, col.screenY2, wallColorU32, 0, 0,
					            col.screenX2, col.screenY2, wallColorU32, 0, 0,
					            col.screenX, col.screenY, wallColorU32, 0, 0);
					AEGfxTriAdd(col.screenX, col.screenY, wallColorU32, 0, 0,
					            col.screenX2, col.screenY2, wallColorU32, 0, 0,
					            col.screenX2, col.screenY, wallColorU32, 0, 0);
					triCount += 2;

					if (triCount >= MAX_TRIANGLES_PER_MESH) {
						AEGfxVertexList* mesh = AEGfxMeshEnd();
						if (mesh) {
							AEGfxSetRenderMode(AE_GFX_RM_COLOR);
							AEGfxSetBlendMode(AE_GFX_BM_NONE);
							AEGfxSetColorToMultiply(1, 1, 1, 1);
							AEGfxSetColorToAdd(0, 0, 0, 0);
							AEMtx33 identity; AEMtx33Identity(&identity); AEGfxSetTransform(identity.m);
							AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
							AEGfxMeshFree(mesh);
						}
						meshIsOpen = false;
					}
				}
			}

			if (meshIsOpen) {
				AEGfxVertexList* mesh = AEGfxMeshEnd();
				if (mesh) {
					if (hasSprite) {
						AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
						AEGfxTextureSet(sprite->GetTexture(), 0, 0);
						AEGfxSetBlendMode(AE_GFX_BM_BLEND);
						AEGfxSetColorToMultiply(1, 1, 1, 1);
						AEGfxSetColorToAdd(0, 0, 0, 0);
						AEGfxSetTransparency(1.0f);
					} else {
						AEGfxSetRenderMode(AE_GFX_RM_COLOR);
						AEGfxSetBlendMode(AE_GFX_BM_NONE);
						AEGfxSetColorToMultiply(1, 1, 1, 1);
						AEGfxSetColorToAdd(0, 0, 0, 0);
					}
					AEMtx33 identity; AEMtx33Identity(&identity); AEGfxSetTransform(identity.m);
					AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
					AEGfxMeshFree(mesh);
				}
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


	void TileMapRenderer::CacheScanlineDistances(int screenHeight, f32 camZ, int horizon) {
		// Only recompute if screen size or camera height or horizon changed
		if (screenHeight == cachedScreenHeight && fabsf(camZ - cachedCamZ) < 0.0001f && horizon == cachedHorizon) {
			return;
		}

		cachedScreenHeight = screenHeight;
		cachedCamZ = camZ;
		cachedHorizon = horizon;
		f32 screenCenterY = screenHeight / 2.0f;

		// Cache ceiling scanlines (from top toward horizon)
		// Ceiling is at height 1.0, so posZ = 1.0 - camZ
		f32 ceilingPosZ = 1.0f - camZ;
		int ceilingRows = (horizon + ROW_STEP - 1) / ROW_STEP + 1;
		ceilingScanlines.resize(ceilingRows);
		
		int idx = 0;
		for (int y = 0; y < horizon; y += ROW_STEP) {
			f32 p = (f32)horizon - (f32)y;
			if (fabsf(p) < 0.1f) {
				ceilingScanlines[idx].rowDist = -1.0f; // Invalid, skip
			} else {
				ceilingScanlines[idx].rowDist = ceilingPosZ * (f32)screenHeight / p;
			}
			ceilingScanlines[idx].aeY = screenCenterY - (f32)y; // aeY2 (top of band)
			idx++;
		}

		// Cache floor scanlines (from horizon toward bottom)
		// Floor is at height 0.0, so posZ = camZ
		f32 floorPosZ = camZ;
		int floorRows = ((screenHeight - horizon) + ROW_STEP - 1) / ROW_STEP + 2; // +2 for final edge
		floorScanlines.resize(floorRows);
		
		idx = 0;
		for (int y = horizon; y <= screenHeight; y += ROW_STEP) {
			f32 p = (f32)y - (f32)horizon;
			if (fabsf(p) < 0.1f) {
				floorScanlines[idx].rowDist = -1.0f; // Invalid, skip
			} else {
				floorScanlines[idx].rowDist = floorPosZ * (f32)screenHeight / p;
			}
			floorScanlines[idx].aeY = screenCenterY - (f32)y; // aeY2 (top of band)
			idx++;
			if (idx >= floorRows) break; // Safety bounds check
		}
	}

} // namespace Engine
