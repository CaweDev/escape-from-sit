/**
 * @file TileMapRenderer.cpp
 * @brief Implementation of TileMapRenderer class
 */
#include "TileMapRenderer.hpp"
#include "ZBuffer.hpp"
#include "Engine/Core/Application.hpp"
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>

// Windows Imaging Component for CPU-side pixel loading
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincodec.h>

namespace Engine {

	TileMapRenderer::TileMapRenderer() : columnMesh(nullptr), fullscreenMesh(nullptr) {}

	TileMapRenderer::~TileMapRenderer() {
		if (columnMesh) {
			AEGfxMeshFree(columnMesh);
		}
		if (fullscreenMesh) {
			AEGfxMeshFree(fullscreenMesh);
		}
	}

	// -------------------------------------------------------------------
	// WIC-based pixel loader: loads a PNG into an RGBA u8 buffer on the CPU
	// -------------------------------------------------------------------
	bool TileMapRenderer::LoadCpuPixels(const std::string& path, CpuTexture& out) {
		out.pixels.clear();
		out.width = out.height = 0;

		CoInitializeEx(nullptr, COINIT_MULTITHREADED);

		IWICImagingFactory* factory = nullptr;
		HRESULT hr = CoCreateInstance(
			CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&factory));
		if (FAILED(hr) || !factory) return false;

		// Convert path to wide string
		int wideLen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
		std::wstring widePath(wideLen, 0);
		MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &widePath[0], wideLen);

		IWICBitmapDecoder* decoder = nullptr;
		hr = factory->CreateDecoderFromFilename(
			widePath.c_str(), nullptr, GENERIC_READ,
			WICDecodeMetadataCacheOnDemand, &decoder);
		if (FAILED(hr) || !decoder) { factory->Release(); return false; }

		IWICBitmapFrameDecode* frame = nullptr;
		hr = decoder->GetFrame(0, &frame);
		if (FAILED(hr) || !frame) { decoder->Release(); factory->Release(); return false; }

		// Convert to 32bpp RGBA
		IWICFormatConverter* converter = nullptr;
		hr = factory->CreateFormatConverter(&converter);
		if (FAILED(hr) || !converter) { frame->Release(); decoder->Release(); factory->Release(); return false; }

		hr = converter->Initialize(
			frame, GUID_WICPixelFormat32bppRGBA,
			WICBitmapDitherTypeNone, nullptr, 0.0,
			WICBitmapPaletteTypeCustom);
		if (FAILED(hr)) { converter->Release(); frame->Release(); decoder->Release(); factory->Release(); return false; }

		UINT w = 0, h = 0;
		converter->GetSize(&w, &h);
		out.width = (int)w;
		out.height = (int)h;
		out.pixels.resize(w * h * 4);

		hr = converter->CopyPixels(nullptr, w * 4, (UINT)out.pixels.size(), out.pixels.data());

		converter->Release();
		frame->Release();
		decoder->Release();
		factory->Release();

		if (FAILED(hr)) { out.pixels.clear(); out.width = out.height = 0; return false; }

		printf("[TileMapRenderer] Loaded CPU texture: %s (%dx%d)\n", path.c_str(), out.width, out.height);
		return true;
	}

	void TileMapRenderer::SetFloorSprite(int tileType, Sprite* sprite) {
		floorSprites[tileType] = sprite;
		if (sprite && sprite->GetTexture2D()) {
			LoadCpuPixels(sprite->GetTexture2D()->GetFilePath(), floorCpuTex);
		}
	}

	void TileMapRenderer::SetCeilingSprite(int tileType, Sprite* sprite) {
		ceilingSprites[tileType] = sprite;
		if (sprite && sprite->GetTexture2D()) {
			LoadCpuPixels(sprite->GetTexture2D()->GetFilePath(), ceilCpuTex);
		}
	}

	void TileMapRenderer::CreateFullscreenMesh() {
		AEGfxMeshStart();
		AEGfxTriAdd(
			-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
			-0.5f,  0.5f, 0xFFFFFFFF, 0.0f, 0.0f,
			 0.5f,  0.5f, 0xFFFFFFFF, 1.0f, 0.0f);
		AEGfxTriAdd(
			-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
			 0.5f,  0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
			 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f);
		fullscreenMesh = AEGfxMeshEnd();
	}

	void TileMapRenderer::Render() {
		if (!enabled) return;

		TileMap* map = gameObject->GetComponent<TileMap>();
		if (!map) return;

		if (!columnMesh) {
			CreateColumnMesh();
		}
		if (!fullscreenMesh) {
			CreateFullscreenMesh();
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

				info.ray = CastRay(pos, info.rayDirX, info.rayDirY, camZ, horizon, screenWidth, screenHeight, map);

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

		// Single-threaded column pass — the column count (~240 at COLUMN_STEP=8) is too
		// small for async overhead to pay off; spawning OS threads per-frame hurts more
		// than it helps, especially on machines with many logical cores.
		ProcessColumnRange(0, numCols);

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
		f32 camZ, int horizon, int screenWidth, int screenHeight, TileMap* map) {
		
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

		// Wall height with aspect ratio correction.
		// Hoist the shared denominator to avoid computing it twice.
		f32 wallDistDenom = result.perpWallDist * cachedPlaneLength * 2.0f;
		f32 wallHeight = (f32)screenWidth / wallDistDenom;
		f32 heightOffset = (camZ - 0.5f) * wallHeight;
		result.drawStart = (f32)horizon - (wallHeight / 2.0f) + heightOffset;
		result.drawEnd = (f32)horizon + (wallHeight / 2.0f) + heightOffset;

		return result;
	}

	// ===================================================================
	// Framebuffer-based ceiling rendering
	// Writes pixels to a CPU buffer, uploads via AEGfxTextureLoadFromMemory,
	// and draws as a single fullscreen quad.
	// ===================================================================
	void TileMapRenderer::RenderCeiling(const Vector2& pos, f32 camZ, int horizon, int screenWidth, int screenHeight, 
		const std::vector<ColumnInfo>& columns, TileMap* map) {
		
		if (ceilingSprites.empty() || columns.empty()) return;
		if (ceilCpuTex.pixels.empty() || ceilCpuTex.width == 0) return;

		// Compute scaled framebuffer dimensions
		int scaledW = (int)(screenWidth * fbScale);
		int scaledH = (int)(screenHeight * fbScale);
		if (scaledW < 1) scaledW = 1;
		if (scaledH < 1) scaledH = 1;

		// Resize framebuffers if needed
		if (fbWidth != scaledW || fbHeight != scaledH) {
			fbWidth = scaledW;
			fbHeight = scaledH;
			ceilFramebuffer.resize(fbWidth * fbHeight * 4, 0);
			floorFramebuffer.resize(fbWidth * fbHeight * 4, 0);
		}

		// Clear ceiling buffer to transparent black
		memset(ceilFramebuffer.data(), 0, ceilFramebuffer.size());

		f32 posZ = 1.0f - camZ;
		f32 rowDistFactor = posZ * (f32)screenWidth / (cachedPlaneLength * 2.0f);

		// Scale horizon to framebuffer space
		f32 fbHorizon = (f32)horizon * fbScale;
		int clampedFbHorizon = (int)fbHorizon;
		if (clampedFbHorizon < 0) clampedFbHorizon = 0;
		if (clampedFbHorizon > fbHeight) clampedFbHorizon = fbHeight;

		const int fogLUTMax = (int)fogLUT.size() - 1;
		const int texW = ceilCpuTex.width;
		const int texH = ceilCpuTex.height;
		const u8* texPixels = ceilCpuTex.pixels.data();
		const int numCols = (int)columns.size();
		const f32 invScale = 1.0f / fbScale;

		for (int fy = 0; fy < clampedFbHorizon; ++fy) {
			// Map framebuffer Y back to screen Y
			f32 screenY = (f32)fy * invScale;
			f32 p = (f32)horizon - screenY;
			if (fabsf(p) < 0.1f) continue;
			f32 rowDist = rowDistFactor / p;
			if (rowDist < 0) continue;

			// Fog for this row
			f32 fogFactor = 1.0f;
			if (fog.enabled) {
				int lutIdx = (int)(rowDist * 100.0f);
				if (lutIdx < 0) lutIdx = 0;
				if (lutIdx > fogLUTMax) lutIdx = fogLUTMax;
				fogFactor = fogLUT[lutIdx];
				if (fogFactor < fog.minVisibility) fogFactor = fog.minVisibility;
				if (fogFactor > fog.maxVisibility) fogFactor = fog.maxVisibility;
			}
			if (fogFactor < 0.01f) break;

			int rowOffset = fy * fbWidth * 4;

			for (int fx = 0; fx < fbWidth; ++fx) {
				// Map framebuffer X back to screen X
				f32 screenX = (f32)fx * invScale;
				int colIdx = (int)(screenX) / COLUMN_STEP;
				if (colIdx >= numCols) colIdx = numCols - 1;

				const auto& col = columns[colIdx];

				// Check if this pixel is behind the wall for this column
				if (screenY >= col.ray.drawStart) continue;

				// Interpolate ray direction within the column block
				f32 t = (screenX - (f32)(colIdx * COLUMN_STEP)) / (f32)COLUMN_STEP;
				f32 rayDirX = col.rayDirX + t * (col.rayDirX2 - col.rayDirX);
				f32 rayDirY = col.rayDirY + t * (col.rayDirY2 - col.rayDirY);

				// World UV
				f32 worldU = pos.x + rowDist * rayDirX;
				f32 worldV = pos.y + rowDist * rayDirY;

				// Texture sampling (wrap via modulo)
				int texX = ((int)floorf(worldU * texW) % texW + texW) % texW;
				int texY = ((int)floorf(worldV * texH) % texH + texH) % texH;

				int texIdx = (texY * texW + texX) * 4;
				int fbIdx = rowOffset + fx * 4;

				ceilFramebuffer[fbIdx + 0] = (u8)(texPixels[texIdx + 0] * fogFactor);
				ceilFramebuffer[fbIdx + 1] = (u8)(texPixels[texIdx + 1] * fogFactor);
				ceilFramebuffer[fbIdx + 2] = (u8)(texPixels[texIdx + 2] * fogFactor);
				ceilFramebuffer[fbIdx + 3] = 255;
			}
		}

		// Upload scaled framebuffer as texture and draw stretched to full screen
		AEGfxTexture* tex = AEGfxTextureLoadFromMemory(ceilFramebuffer.data(), (u32)fbWidth, (u32)fbHeight);
		if (tex && fullscreenMesh) {
			AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
			AEGfxTextureSet(tex, 0, 0);
			AEGfxSetBlendMode(AE_GFX_BM_BLEND);
			AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
			AEGfxSetColorToAdd(0, 0, 0, 0);
			AEGfxSetTransparency(1.0f);

			AEMtx33 scale, transform;
			AEMtx33Scale(&scale, (f32)screenWidth, (f32)screenHeight);
			AEMtx33Identity(&transform);
			AEMtx33Concat(&transform, &transform, &scale);
			AEGfxSetTransform(transform.m);
			AEGfxMeshDraw(fullscreenMesh, AE_GFX_MDM_TRIANGLES);
			AEGfxTextureUnload(tex);
		}
	}
	// ===================================================================
	// Framebuffer-based floor rendering
	// ===================================================================
	void TileMapRenderer::RenderFloor(const Vector2& pos, f32 camZ, int horizon, int screenWidth, int screenHeight, 
		const std::vector<ColumnInfo>& columns, TileMap* map) {
		
		if (floorSprites.empty() || columns.empty()) return;
		if (floorCpuTex.pixels.empty() || floorCpuTex.width == 0) return;

		// Compute scaled framebuffer dimensions
		int scaledW = (int)(screenWidth * fbScale);
		int scaledH = (int)(screenHeight * fbScale);
		if (scaledW < 1) scaledW = 1;
		if (scaledH < 1) scaledH = 1;

		// Resize framebuffers if needed
		if (fbWidth != scaledW || fbHeight != scaledH) {
			fbWidth = scaledW;
			fbHeight = scaledH;
			floorFramebuffer.resize(fbWidth * fbHeight * 4, 0);
			ceilFramebuffer.resize(fbWidth * fbHeight * 4, 0);
		}

		// Clear floor buffer to transparent black
		memset(floorFramebuffer.data(), 0, floorFramebuffer.size());

		f32 posZ = camZ;
		f32 rowDistFactor = posZ * (f32)screenWidth / (cachedPlaneLength * 2.0f);

		// Scale horizon to framebuffer space
		f32 fbHorizon = (f32)horizon * fbScale;
		int clampedFbHorizon = (int)fbHorizon;
		if (clampedFbHorizon < 0) clampedFbHorizon = 0;
		if (clampedFbHorizon > fbHeight) clampedFbHorizon = fbHeight;

		const f32 invScale = 1.0f / fbScale;

		// Skip invisible far rows near horizon
		int startFy = clampedFbHorizon;
		if (fog.enabled && posZ > 0.001f) {
			f32 maxVisibleDist = fog.distance * 4.6f;
			f32 minP = rowDistFactor / maxVisibleDist;
			int computedScreenStart = horizon + (int)ceilf(minP);
			int computedFbStart = (int)(computedScreenStart * fbScale);
			if (computedFbStart > clampedFbHorizon) {
				startFy = computedFbStart;
			}
		}

		const int fogLUTMax = (int)fogLUT.size() - 1;
		const int texW = floorCpuTex.width;
		const int texH = floorCpuTex.height;
		const u8* texPixels = floorCpuTex.pixels.data();
		const int numCols = (int)columns.size();

		for (int fy = startFy; fy < fbHeight; ++fy) {
			// Map framebuffer Y back to screen Y
			f32 screenY = (f32)fy * invScale;
			f32 p = screenY - (f32)horizon;
			if (fabsf(p) < 0.1f) continue;
			f32 rowDist = rowDistFactor / p;
			if (rowDist < 0) continue;

			// Fog for this row
			f32 fogFactor = 1.0f;
			if (fog.enabled) {
				int lutIdx = (int)(rowDist * 100.0f);
				if (lutIdx < 0) lutIdx = 0;
				if (lutIdx > fogLUTMax) lutIdx = fogLUTMax;
				fogFactor = fogLUT[lutIdx];
				if (fogFactor < fog.minVisibility) fogFactor = fog.minVisibility;
				if (fogFactor > fog.maxVisibility) fogFactor = fog.maxVisibility;
			}
			if (fogFactor < 0.01f) continue;

			int rowOffset = fy * fbWidth * 4;

			for (int fx = 0; fx < fbWidth; ++fx) {
				// Map framebuffer X back to screen X
				f32 screenX = (f32)fx * invScale;
				int colIdx = (int)(screenX) / COLUMN_STEP;
				if (colIdx >= numCols) colIdx = numCols - 1;

				const auto& col = columns[colIdx];

				// Check if this pixel is behind the wall for this column
				if (screenY < col.ray.drawEnd) continue;

				// Interpolate ray direction within the column block
				f32 t = (screenX - (f32)(colIdx * COLUMN_STEP)) / (f32)COLUMN_STEP;
				f32 rayDirX = col.rayDirX + t * (col.rayDirX2 - col.rayDirX);
				f32 rayDirY = col.rayDirY + t * (col.rayDirY2 - col.rayDirY);

				// World UV
				f32 worldU = pos.x + rowDist * rayDirX;
				f32 worldV = pos.y + rowDist * rayDirY;

				// Texture sampling (wrap via modulo)
				int texX = ((int)floorf(worldU * texW) % texW + texW) % texW;
				int texY = ((int)floorf(worldV * texH) % texH + texH) % texH;

				int texIdx = (texY * texW + texX) * 4;
				int fbIdx = rowOffset + fx * 4;

				floorFramebuffer[fbIdx + 0] = (u8)(texPixels[texIdx + 0] * fogFactor);
				floorFramebuffer[fbIdx + 1] = (u8)(texPixels[texIdx + 1] * fogFactor);
				floorFramebuffer[fbIdx + 2] = (u8)(texPixels[texIdx + 2] * fogFactor);
				floorFramebuffer[fbIdx + 3] = 255;
			}
		}

		// Upload scaled framebuffer as texture and draw stretched to full screen
		AEGfxTexture* tex = AEGfxTextureLoadFromMemory(floorFramebuffer.data(), (u32)fbWidth, (u32)fbHeight);
		if (tex && fullscreenMesh) {
			AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
			AEGfxTextureSet(tex, 0, 0);
			AEGfxSetBlendMode(AE_GFX_BM_BLEND);
			AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
			AEGfxSetColorToAdd(0, 0, 0, 0);
			AEGfxSetTransparency(1.0f);

			AEMtx33 scale, transform;
			AEMtx33Scale(&scale, (f32)screenWidth, (f32)screenHeight);
			AEMtx33Identity(&transform);
			AEMtx33Concat(&transform, &transform, &scale);
			AEGfxSetTransform(transform.m);
			AEGfxMeshDraw(fullscreenMesh, AE_GFX_MDM_TRIANGLES);
			AEGfxTextureUnload(tex);
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

