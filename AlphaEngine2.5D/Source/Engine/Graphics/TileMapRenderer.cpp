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
			ColumnInfo info;
			info.rayDirX = dirX + planeX * cameraX;
			info.rayDirY = dirY + planeY * cameraX;

			info.ray = CastRay(pos, info.rayDirX, info.rayDirY, camZ, screenHeight, map);

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

		// Render in order: ceiling, floor, walls (walls on top)
		RenderCeiling(pos, camZ, screenWidth, screenHeight, columns, map);
		RenderFloor(pos, camZ, screenWidth, screenHeight, columns, map);
		RenderWalls(screenHeight, columns);
	}

	TileMapRenderer::RaycastResult TileMapRenderer::CastRay(const Vector2& pos, f32 rayDirX, f32 rayDirY, 
		f32 camZ, int screenHeight, TileMap* map) {
		
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
		result.drawStart = (screenHeight - wallHeight) / 2.0f + heightOffset;
		result.drawEnd = (screenHeight + wallHeight) / 2.0f + heightOffset;

		return result;
	}

	void TileMapRenderer::RenderCeiling(const Vector2& pos, f32 camZ, int screenWidth, int screenHeight, 
		const std::vector<ColumnInfo>& columns, TileMap* map) {
		
		// Check if we have any ceiling sprites
		if (ceilingSprites.empty()) return;

		// Pre-calculate row distances to avoid divisions in the inner loop
		std::vector<f32> rowDists(screenHeight);
		f32 posZ = 1.0f - camZ;
		for (int y = 0; y < screenHeight; ++y) {
			f32 p = (f32)(screenHeight / 2.0f) - (f32)y;
			if (fabsf(p) < 0.1f) rowDists[y] = -1.0f;
			else rowDists[y] = posZ * (f32)screenHeight / p;
		}

		int triCount = 0;
		bool meshIsOpen = false;
		
		for (const auto& col : columns) {
			f32 drawStart = col.ray.drawStart < 1 ? 1 : col.ray.drawStart;
			
			for (int y = 0; y < (int)drawStart; y += ROW_STEP) {
				if (!meshIsOpen) {
					AEGfxMeshStart();
					meshIsOpen = true;
					triCount = 0;
				}

				int yNext = y + ROW_STEP;
				if (yNext > (int)drawStart) yNext = (int)drawStart;
				
				f32 rowDist1 = rowDists[y];
				if (rowDist1 < 0) continue;
				
				f32 u1 = pos.x + rowDist1 * col.rayDirX;
				f32 v1 = pos.y + rowDist1 * col.rayDirY;
				
				f32 rowDist2 = rowDists[yNext < screenHeight ? yNext : screenHeight - 1];
				f32 u2 = u1, v2 = v1;
				if (rowDist2 > 0) {
					u2 = pos.x + rowDist2 * col.rayDirX;
					v2 = pos.y + rowDist2 * col.rayDirY;
				}
				
				f32 aeY1 = screenHeight / 2.0f - (f32)yNext;
				f32 aeY2 = screenHeight / 2.0f - (f32)y;
				
				AEGfxTriAdd(col.screenX, aeY2, 0xFFFFFFFF, u1, v1, col.screenX2, aeY2, 0xFFFFFFFF, u1, v1, col.screenX, aeY1, 0xFFFFFFFF, u2, v2);
				AEGfxTriAdd(col.screenX, aeY1, 0xFFFFFFFF, u2, v2, col.screenX2, aeY2, 0xFFFFFFFF, u1, v1, col.screenX2, aeY1, 0xFFFFFFFF, u2, v2);
				triCount += 2;

				if (triCount >= MAX_TRIANGLES_PER_MESH) {
					AEGfxVertexList* mesh = AEGfxMeshEnd();
					if (mesh) {
						auto sprite = GetCeilingSprite(1);
						if (sprite && sprite->IsLoaded()) {
							AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
							AEGfxTextureSet(sprite->GetTexture(), 0, 0);
						AEGfxSetBlendMode(AE_GFX_BM_BLEND);
						AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
						AEGfxSetColorToAdd(0, 0, 0, 0);
						AEGfxSetTransparency(1.0f);
						AEMtx33 identity; AEMtx33Identity(&identity); AEGfxSetTransform(identity.m);
						AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
					}
					AEGfxMeshFree(mesh);
				}
				meshIsOpen = false;
				}
			}
		}
		
		if (meshIsOpen) {
			AEGfxVertexList* mesh = AEGfxMeshEnd();
			if (mesh) {
				auto sprite = GetCeilingSprite(1);
				if (sprite && sprite->IsLoaded()) {
					AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
					AEGfxTextureSet(sprite->GetTexture(), 0, 0);
				AEGfxSetBlendMode(AE_GFX_BM_BLEND);
				AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
				AEGfxSetColorToAdd(0, 0, 0, 0);
				AEGfxSetTransparency(1.0f);
				AEMtx33 identity; AEMtx33Identity(&identity); AEGfxSetTransform(identity.m);
				AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
			}
			AEGfxMeshFree(mesh);
		}
		}
	}

	void TileMapRenderer::RenderFloor(const Vector2& pos, f32 camZ, int screenWidth, int screenHeight, 
		const std::vector<ColumnInfo>& columns, TileMap* map) {
		
		// Check if we have any floor sprites
		if (floorSprites.empty()) return;

		// Pre-calculate row distances
		std::vector<f32> rowDists(screenHeight);
		f32 posZ = camZ;
		for (int y = 0; y < screenHeight; ++y) {
			f32 p = (f32)y - (f32)(screenHeight / 2.0f);
			if (fabsf(p) < 0.1f) rowDists[y] = -1.0f;
			else rowDists[y] = posZ * (f32)screenHeight / p;
		}

		int triCount = 0;
		bool meshIsOpen = false;
		
		for (const auto& col : columns) {
			f32 drawEnd = col.ray.drawEnd >= screenHeight - 1 ? (f32)screenHeight - 1 : col.ray.drawEnd;
			
			for (int y = (int)drawEnd; y < screenHeight; y += ROW_STEP) {
				if (!meshIsOpen) {
					AEGfxMeshStart();
					meshIsOpen = true;
					triCount = 0;
				}

				int yNext = y + ROW_STEP;
				if (yNext > screenHeight) yNext = screenHeight;
				
				f32 rowDist1 = rowDists[y];
				if (rowDist1 < 0) continue;
				
				f32 u1 = pos.x + rowDist1 * col.rayDirX;
				f32 v1 = pos.y + rowDist1 * col.rayDirY;
				
				f32 rowDist2 = rowDists[yNext < screenHeight ? yNext : screenHeight - 1];
				f32 u2 = u1, v2 = v1;
				if (rowDist2 > 0) {
					u2 = pos.x + rowDist2 * col.rayDirX;
					v2 = pos.y + rowDist2 * col.rayDirY;
				}
				
				f32 aeY1 = screenHeight / 2.0f - (f32)yNext;
				f32 aeY2 = screenHeight / 2.0f - (f32)y;
				
				AEGfxTriAdd(col.screenX, aeY2, 0xFFFFFFFF, u1, v1, col.screenX2, aeY2, 0xFFFFFFFF, u1, v1, col.screenX, aeY1, 0xFFFFFFFF, u2, v2);
				AEGfxTriAdd(col.screenX, aeY1, 0xFFFFFFFF, u2, v2, col.screenX2, aeY2, 0xFFFFFFFF, u1, v1, col.screenX2, aeY1, 0xFFFFFFFF, u2, v2);
				triCount += 2;

				if (triCount >= MAX_TRIANGLES_PER_MESH) {
					AEGfxVertexList* mesh = AEGfxMeshEnd();
					if (mesh) {
						auto sprite = GetFloorSprite(1);
						if (sprite && sprite->IsLoaded()) {
							AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
							AEGfxTextureSet(sprite->GetTexture(), 0, 0);
							AEGfxSetBlendMode(AE_GFX_BM_BLEND);
							AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
							AEGfxSetColorToAdd(0, 0, 0, 0);
							AEGfxSetTransparency(1.0f);
							AEMtx33 identity; AEMtx33Identity(&identity); AEGfxSetTransform(identity.m);
							AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
						}
						AEGfxMeshFree(mesh);
					}
					meshIsOpen = false;
				}
			}
		}

		if (meshIsOpen) {
			AEGfxVertexList* mesh = AEGfxMeshEnd();
			if (mesh) {
				auto sprite = GetFloorSprite(1);
				if (sprite && sprite->IsLoaded()) {
					AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
					AEGfxTextureSet(sprite->GetTexture(), 0, 0);
					AEGfxSetBlendMode(AE_GFX_BM_BLEND);
					AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
					AEGfxSetColorToAdd(0, 0, 0, 0);
					AEGfxSetTransparency(1.0f);
					AEMtx33 identity; AEMtx33Identity(&identity); AEGfxSetTransform(identity.m);
					AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
				}
				AEGfxMeshFree(mesh);
			}
		}
	}

	void TileMapRenderer::RenderWalls(int screenHeight, const std::vector<ColumnInfo>& columns) {
		
		bool hasCeilingSprites = !ceilingSprites.empty();
		bool hasFloorSprites = !floorSprites.empty();

		// Sequential Batch 1: Ceiling Fallback
		if (!hasCeilingSprites) {
			u32 ceilColorU32 = (((u32)(ceilingColor.r * 255)) << 24) | (((u32)(ceilingColor.g * 255)) << 16) | (((u32)(ceilingColor.b * 255)) << 8) | (u32)(ceilingColor.a * 255);
			int triCount = 0;
			bool meshIsOpen = false;
			
			for (const auto& col : columns) {
				f32 topY = screenHeight / 2.0f;
				f32 wallStart = col.screenY2;
				for (f32 y = topY; y > wallStart; y -= ROW_STEP) {
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
			
			for (const auto& col : columns) {
				f32 wallEnd = col.screenY;
				f32 bottomY = -screenHeight / 2.0f;
				for (f32 y = wallEnd; y > bottomY; y -= ROW_STEP) {
					if (!meshIsOpen) {
						AEGfxMeshStart();
						meshIsOpen = true;
						triCount = 0;
					}

					f32 yNext = y - ROW_STEP;
					if (yNext < bottomY) yNext = bottomY;
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
					for (f32 y = col.screenY2; y > col.screenY; y -= ROW_STEP) {
						if (!meshIsOpen) {
							AEGfxMeshStart();
							meshIsOpen = true;
							triCount = 0;
						}

						f32 yNext = y - ROW_STEP;
						if (yNext < col.screenY) yNext = col.screenY;
						f32 h = col.screenY2 - col.screenY;
						if (h < 0.1f) h = 0.1f;
						f32 v1 = col.vTop + ((col.screenY2 - y) / h) * (col.vBottom - col.vTop);
						f32 v2 = col.vTop + ((col.screenY2 - yNext) / h) * (col.vBottom - col.vTop);
						AEGfxTriAdd(col.screenX, y, fogColor, col.ray.wallX, v1, col.screenX2, y, fogColor, col.ray.wallX, v1, col.screenX, yNext, fogColor, col.ray.wallX, v2);
						AEGfxTriAdd(col.screenX, yNext, fogColor, col.ray.wallX, v2, col.screenX2, y, fogColor, col.ray.wallX, v1, col.screenX2, yNext, fogColor, col.ray.wallX, v2);
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
					}
				} else {
					Color c = (col.ray.side == 0) ? wallColorNS : wallColorEW;
					u32 wallColorU32 = (((u32)(c.r * col.fogFactor * 255)) << 24) | (((u32)(c.g * col.fogFactor * 255)) << 16) | (((u32)(c.b * col.fogFactor * 255)) << 8) | 0xFF;
					for (f32 y = col.screenY2; y > col.screenY; y -= ROW_STEP) {
						if (!meshIsOpen) {
							AEGfxMeshStart();
							meshIsOpen = true;
							triCount = 0;
						}

						f32 yNext = y - ROW_STEP;
						if (yNext < col.screenY) yNext = col.screenY;
						AEGfxTriAdd(col.screenX, y, wallColorU32, 0, 0, col.screenX2, y, wallColorU32, 0, 0, col.screenX, yNext, wallColorU32, 0, 0);
						AEGfxTriAdd(col.screenX, yNext, wallColorU32, 0, 0, col.screenX2, y, wallColorU32, 0, 0, col.screenX2, yNext, wallColorU32, 0, 0);
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


} // namespace Engine
