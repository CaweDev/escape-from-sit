/**
 * @file TileMapRenderer.cpp
 * @brief Implementation of TileMapRenderer class
 */
#include "TileMapRenderer.hpp"
#include "ZBuffer.hpp"
#include <cmath>

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

		// Lazy init mesh
		if (!columnMesh) {
			CreateColumnMesh();
		}

		int screenWidth = AEGfxGetWindowWidth();
		int screenHeight = AEGfxGetWindowHeight();

		// Update Z-buffer
		ZBuffer::Resize(screenWidth);
		ZBuffer::Clear();

		// Use camera transform if available, otherwise fall back to own transform
		Transform* camTransform = (Camera::main) ? Camera::main->transform : transform;
		if (!camTransform) return;

		Vector2 pos = Vector2(camTransform->position.x, camTransform->position.y);
		f32 angleRad = camTransform->rotation * 3.14159f / 180.0f;

		// Direction and camera plane vectors (standard raycasting approach)
		f32 dirX = cosf(angleRad);
		f32 dirY = sinf(angleRad);

		// Plane is perpendicular to direction, length determined by FOV
		// planeLength = tan(fovRad / 2)
		f32 planeLength = tanf((fov * 3.14159f / 180.0f) / 2.0f);
		f32 planeX = -dirY * planeLength;
		f32 planeY = dirX * planeLength;

		// Draw ceiling
		DrawRect(0, 0, (f32)screenWidth, (f32)screenHeight / 2.0f, ceilingColor, screenWidth, screenHeight);

		// Draw floor  
		DrawRect(0, (f32)screenHeight / 2.0f, (f32)screenWidth, (f32)screenHeight / 2.0f, floorColor, screenWidth, screenHeight);

		for (int x = 0; x < screenWidth; ++x) {
			// Calculate ray position and direction
			f32 cameraX = 2.0f * x / (f32)screenWidth - 1.0f; // x-coordinate in camera space
			f32 rayDirX = dirX + planeX * cameraX;
			f32 rayDirY = dirY + planeY * cameraX;

			// Which box of the map we're in
			int mapX = (int)pos.x;
			int mapY = (int)pos.y;

			// Length of ray from one x or y-side to next x or y-side
			f32 deltaDistX = (rayDirX == 0) ? 1e30f : fabsf(1.0f / rayDirX);
			f32 deltaDistY = (rayDirY == 0) ? 1e30f : fabsf(1.0f / rayDirY);

			f32 sideDistX, sideDistY;

			// What direction to step in x or y-direction (either +1 or -1)
			int stepX, stepY;

			// Calculate step and initial sideDist
			if (rayDirX < 0) {
				stepX = -1;
				sideDistX = (pos.x - mapX) * deltaDistX;
			}
			else {
				stepX = 1;
				sideDistX = (mapX + 1.0f - pos.x) * deltaDistX;
			}

			if (rayDirY < 0) {
				stepY = -1;
				sideDistY = (pos.y - mapY) * deltaDistY;
			}
			else {
				stepY = 1;
				sideDistY = (mapY + 1.0f - pos.y) * deltaDistY;
			}

			bool hit = false;
			int side = 0; // Was a NS or a EW wall hit?

			// DDA algorithm
			while (!hit) {
				// Jump to next map square, either in x-direction, or in y-direction
				if (sideDistX < sideDistY) {
					sideDistX += deltaDistX;
					mapX += stepX;
					side = 0;
				}
				else {
					sideDistY += deltaDistY;
					mapY += stepY;
					side = 1;
				}

				// Check if ray has hit a wall
				if (map->IsWall(mapX, mapY)) {
					hit = true;
				}
			}

			// Calculate distance projected on camera direction (Euclidean distance will give fisheye effect!)
			f32 perpWallDist;
			if (side == 0) perpWallDist = (sideDistX - deltaDistX);
			else           perpWallDist = (sideDistY - deltaDistY);

			if (perpWallDist < 0.1f) perpWallDist = 0.1f; // Avoid division by zero

			// Calculate height of line to draw on screen
			f32 wallHeight = (f32)screenHeight / perpWallDist;

			// Calculate lowest and highest pixel to fill in current stripe
			f32 drawStart = (screenHeight - wallHeight) / 2.0f;
			if (drawStart < 0) drawStart = 0;

			f32 drawEnd = (screenHeight + wallHeight) / 2.0f;
			if (drawEnd >= screenHeight) drawEnd = (f32)screenHeight - 1;

			// Choose wall color
			Color wallColor = (side == 0) ? wallColorNS : wallColorEW;

			// Simple depth shading (fog)
			f32 fogFactor = 1.0f - (perpWallDist / 20.0f);
			if (fogFactor < 0.2f) fogFactor = 0.2f;
			wallColor.r *= fogFactor;
			wallColor.g *= fogFactor;
			wallColor.b *= fogFactor;

			DrawColumn((f32)x, drawStart, drawEnd - drawStart, wallColor, screenWidth, screenHeight);
			
			// Store depth in Z-buffer
			ZBuffer::SetDepth(x, perpWallDist);
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

	void TileMapRenderer::DrawRect(f32 x, f32 y, f32 w, f32 h, const Color& color, int sw, int sh) {
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		AEGfxSetBlendMode(AE_GFX_BM_NONE);
		AEGfxSetColorToMultiply(color.r, color.g, color.b, color.a);
		AEGfxSetColorToAdd(0, 0, 0, 0);

		AEMtx33 scale, trans, result;
		AEMtx33Scale(&scale, w, h);
		f32 screenX = x - sw / 2.0f;
		f32 screenY = sh / 2.0f - y - h;
		AEMtx33Trans(&trans, screenX, screenY);
		AEMtx33Concat(&result, &trans, &scale);
		AEGfxSetTransform(result.m);
		AEGfxMeshDraw(columnMesh, AE_GFX_MDM_TRIANGLES);
	}

} // namespace Engine
