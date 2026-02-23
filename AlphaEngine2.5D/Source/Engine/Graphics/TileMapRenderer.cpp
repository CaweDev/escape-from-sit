/**
 * @file TileMapRenderer.cpp
 * @brief Implementation of the TileMapRenderer raycasting renderer.
 *
 * Rendering pipeline per frame:
 *   1. Column pass  — DDA-casts one ray per COLUMN_STEP pixels, fills managedColumns
 *                     and writes perpendicular distances to ZBuffer.
 *   2. Ceiling pass — CPU-rasterises ceiling into a scaled framebuffer, uploads as
 *                     a single GPU texture, draws one full-screen quad.
 *   3. Floor pass   — Same as ceiling but below the horizon.
 *   4. Wall pass    — Draws textured/coloured wall strips via AEGfxTriAdd, batched
 *                     by tile type and fog key to minimise GPU draw calls.
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

	// -----------------------------------------------------------------------
	// WIC pixel loader
	// Loads a PNG from disk into a RGBA8 CPU buffer using Windows Imaging
	// Component (WIC). Called once when SetFloor/CeilingSprite is called,
	// not every frame. COM is initialised on the calling thread if needed.
	// -----------------------------------------------------------------------
	bool TileMapRenderer::LoadCpuPixels(const std::string& path, CpuTexture& out) {
		out.pixels.clear();
		out.width = out.height = 0;

		// Ensure COM is initialised for this thread (safe to call multiple times).
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);

		// Create the WIC factory — our entry point for all WIC operations.
		IWICImagingFactory* factory = nullptr;
		HRESULT hr = CoCreateInstance(
			CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&factory));
		if (FAILED(hr) || !factory) return false;

		// Convert the UTF-8 file path to a wide string for the WIC API.
		int wideLen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
		std::wstring widePath(wideLen, 0);
		MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &widePath[0], wideLen);

		// Decode the image file into a WIC bitmap.
		IWICBitmapDecoder* decoder = nullptr;
		hr = factory->CreateDecoderFromFilename(
			widePath.c_str(), nullptr, GENERIC_READ,
			WICDecodeMetadataCacheOnDemand, &decoder);
		if (FAILED(hr) || !decoder) { factory->Release(); return false; }

		// Grab frame 0 (PNG files have exactly one frame).
		IWICBitmapFrameDecode* frame = nullptr;
		hr = decoder->GetFrame(0, &frame);
		if (FAILED(hr) || !frame) { decoder->Release(); factory->Release(); return false; }

		// Convert whatever pixel format the file uses into 32bpp RGBA.
		IWICFormatConverter* converter = nullptr;
		hr = factory->CreateFormatConverter(&converter);
		if (FAILED(hr) || !converter) { frame->Release(); decoder->Release(); factory->Release(); return false; }

		hr = converter->Initialize(
			frame, GUID_WICPixelFormat32bppRGBA,
			WICBitmapDitherTypeNone, nullptr, 0.0,
			WICBitmapPaletteTypeCustom);
		if (FAILED(hr)) { converter->Release(); frame->Release(); decoder->Release(); factory->Release(); return false; }

		// Copy the converted pixel data into our CPU buffer.
		UINT w = 0, h = 0;
		converter->GetSize(&w, &h);
		out.width = (int)w;
		out.height = (int)h;
		out.pixels.resize(w * h * 4);

		hr = converter->CopyPixels(nullptr, w * 4, (UINT)out.pixels.size(), out.pixels.data());

		// Release all WIC COM objects (order: converter → frame → decoder → factory).
		converter->Release();
		frame->Release();
		decoder->Release();
		factory->Release();

		if (FAILED(hr)) { out.pixels.clear(); out.width = out.height = 0; return false; }

		printf("[TileMapRenderer] Loaded CPU texture: %s (%dx%d)\n", path.c_str(), out.width, out.height);
		return true;
	}

	// Stores the sprite pointer and immediately loads a CPU-side pixel copy.
	// The CPU copy is required by the framebuffer rasteriser since we cannot
	// read pixels back from an AEGfxTexture (GPU-only handle).
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

	// Full-screen unit quad centred at origin (vertex positions in [-0.5, 0.5]).
	// UV (0,0) maps to top-left of the framebuffer texture; (1,1) to bottom-right.
	void TileMapRenderer::CreateFullscreenMesh() {
		AEGfxMeshStart();
		AEGfxTriAdd(
			-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,   // bottom-left
			-0.5f,  0.5f, 0xFFFFFFFF, 0.0f, 0.0f,   // top-left
			 0.5f,  0.5f, 0xFFFFFFFF, 1.0f, 0.0f);  // top-right
		AEGfxTriAdd(
			-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,   // bottom-left
			 0.5f,  0.5f, 0xFFFFFFFF, 1.0f, 0.0f,   // top-right
			 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f);  // bottom-right
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

	TileMapRenderer::RaycastResult TileMapRenderer::CastRay(
		const Vector2& pos, f32 rayDirX, f32 rayDirY,
		f32 camZ, int horizon, int screenWidth, int screenHeight, TileMap* map) {

		RaycastResult result;

		// --- DDA setup ---
		// Start at the tile that contains the camera position.
		int mapX = (int)floorf(pos.x); // Grid column the ray starts in.
		int mapY = (int)floorf(pos.y); // Grid row the ray starts in.

		// deltaDistX/Y = how far the ray travels along the ray direction to cross one full tile on each axis.
		// If the ray is parallel to an axis (dir == 0) the distance is effectively infinite.
		f32 deltaDistX = (rayDirX == 0) ? 1e30f : fabsf(1.0f / rayDirX);
		f32 deltaDistY = (rayDirY == 0) ? 1e30f : fabsf(1.0f / rayDirY);

		f32 sideDistX, sideDistY; // Distance to the very next X / Y grid line from the ray origin.
		int stepX, stepY;         // Which direction to step (+1 or -1) along each axis.

		// Initialise step direction and distance-to-first-grid-crossing for X.
		if (rayDirX < 0) { stepX = -1; sideDistX = (pos.x - mapX) * deltaDistX; }      // Moving left.
		else             { stepX =  1; sideDistX = (mapX + 1.0f - pos.x) * deltaDistX; } // Moving right.

		// Same for Y axis.
		if (rayDirY < 0) { stepY = -1; sideDistY = (pos.y - mapY) * deltaDistY; }       // Moving up.
		else             { stepY =  1; sideDistY = (mapY + 1.0f - pos.y) * deltaDistY; } // Moving down.

		// --- DDA traversal ---
		// Advance through grid cells until we hit a wall tile.
		// Each iteration steps to the next grid crossing on whichever axis is closer.
		result.side = 0;
		while (!map->IsWall(mapX, mapY)) {
			if (sideDistX < sideDistY) {
				sideDistX += deltaDistX; // Advance X side distance by one tile width.
				mapX      += stepX;      // Move to the next tile column.
				result.side = 0;         // Hit was on an X (N/S) boundary.
			} else {
				sideDistY += deltaDistY; // Advance Y side distance by one tile height.
				mapY      += stepY;      // Move to the next tile row.
				result.side = 1;         // Hit was on a Y (E/W) boundary.
			}
		}

		// Record which tile type was struck (used to select the correct wall sprite).
		result.tileType = static_cast<int>(map->GetTile(mapX, mapY));

		// Perpendicular wall distance: subtract one step back to undo the overshoot.
		// Using the perpendicular (not Euclidean) distance avoids the "fisheye" distortion.
		result.perpWallDist = (result.side == 0)
			? (sideDistX - deltaDistX)  // X-boundary hit.
			: (sideDistY - deltaDistY); // Y-boundary hit.
		if (result.perpWallDist < 0.1f) result.perpWallDist = 0.1f; // Clamp to avoid divide-by-zero / z-fighting.

		// Compute the exact world-space coordinate along the wall face that the ray struck.
		// This becomes the U texture coordinate for horizontal texture wrapping.
		if (result.side == 0) {
			result.wallX = pos.y + result.perpWallDist * rayDirY; // X-boundary: use Y world coordinate.
		} else {
			result.wallX = pos.x + result.perpWallDist * rayDirX; // Y-boundary: use X world coordinate.
		}
		// wallX is the absolute world position; the renderer takes frac(wallX) to get [0,1] UV.

		// --- Screen-space wall extents ---
		// Wall height in pixels = screenWidth / (perpWallDist * planeLength * 2).
		// Dividing screenWidth (not screenHeight) gives correct aspect-ratio scaling.
		f32 wallDistDenom = result.perpWallDist * cachedPlaneLength * 2.0f; // Shared denominator (hoisted).
		f32 wallHeight    = (f32)screenWidth / wallDistDenom; // Projected wall height in pixels.

		// camZ in [0,1]: 0.5 = eye-level. Offset the wall vertically by the camera's height.
		f32 heightOffset = (camZ - 0.5f) * wallHeight;

		// drawStart / drawEnd are screen-space Y pixel positions (can be negative or > screenHeight).
		result.drawStart = (f32)horizon - (wallHeight / 2.0f) + heightOffset; // Top of the wall column.
		result.drawEnd   = (f32)horizon + (wallHeight / 2.0f) + heightOffset; // Bottom of the wall column.

		return result;
	}

	// ===================================================================
	// Framebuffer-based ceiling rendering
	//
	// Strategy: For each framebuffer pixel above the horizon line, project
	// the pixel onto the world plane (z = ceiling height), sample the
	// ceiling texture at that world coordinate, apply fog, and write the
	// result into a CPU pixel buffer. After the loop, upload the buffer
	// as a transient GPU texture and draw it with one fullscreen quad.
	// ===================================================================
	void TileMapRenderer::RenderCeiling(const Vector2& pos, f32 camZ, int horizon, int screenWidth, int screenHeight, 
		const std::vector<ColumnInfo>& columns, TileMap* map) {
		
		if (ceilingSprites.empty() || columns.empty()) return; // No ceiling sprite assigned — nothing to draw.
		if (ceilCpuTex.pixels.empty() || ceilCpuTex.width == 0) return; // WIC pixel load failed — skip.

		// Compute the internal framebuffer resolution (smaller → faster, blurrier).
		int scaledW = (int)(screenWidth * fbScale);  // Framebuffer width in pixels.
		int scaledH = (int)(screenHeight * fbScale);  // Framebuffer height in pixels.
		if (scaledW < 1) scaledW = 1; // Safety clamp.
		if (scaledH < 1) scaledH = 1;

		// Re-allocate BOTH framebuffers if the dimensions have changed (e.g. window resize or fbScale change).
		// Both buffers share fbWidth/fbHeight, so whichever function runs first must resize both.
		if (fbWidth != scaledW || fbHeight != scaledH) {
			fbWidth = scaledW;
			fbHeight = scaledH;
			ceilFramebuffer.resize(fbWidth * fbHeight * 4, 0);  // 4 bytes per pixel (RGBA).
			floorFramebuffer.resize(fbWidth * fbHeight * 4, 0);
		}

		// Zero out the entire buffer — unwritten pixels will be transparent black (alpha = 0),
		// so walls and other geometry can show through when the quad is drawn with blending.
		memset(ceilFramebuffer.data(), 0, ceilFramebuffer.size());

		// posZ is the vertical distance from the camera to the ceiling plane.
		// camZ ∈ [0, 1]: at camZ = 0 (ground), posZ = 1 (full ceiling height above).
		f32 posZ = 1.0f - camZ;

		// rowDistFactor converts a screen-space "p" value (pixels from horizon) into a
		// world-space distance to the point on the ceiling plane directly below/above.
		// Formula: rowDist = (posZ * screenWidth) / (2 * planeLength * p)
		//        = rowDistFactor / p
		f32 rowDistFactor = posZ * (f32)screenWidth / (cachedPlaneLength * 2.0f);

		// Map the screen-space horizon to framebuffer space. Everything above the
		// horizon is ceiling; we only iterate framebuffer rows [0, clampedFbHorizon).
		f32 fbHorizon = (f32)horizon * fbScale;
		int clampedFbHorizon = (int)fbHorizon;
		if (clampedFbHorizon < 0) clampedFbHorizon = 0;           // Horizon above screen → no ceiling visible.
		if (clampedFbHorizon > fbHeight) clampedFbHorizon = fbHeight; // Horizon below screen → entire screen is ceiling.

		// Cache frequently-accessed values outside the loop to avoid repeated lookups.
		const int fogLUTMax = (int)fogLUT.size() - 1; // Max valid index into the fog LUT.
		const int texW = ceilCpuTex.width;             // Ceiling texture width in texels.
		const int texH = ceilCpuTex.height;            // Ceiling texture height in texels.
		const u8* texPixels = ceilCpuTex.pixels.data(); // Raw RGBA pixel data pointer.
		const int numCols = (int)columns.size();        // Number of raycast column blocks.
		const f32 invScale = 1.0f / fbScale;            // Multiplier to map fb coords → screen coords.

		// --- Row loop: iterate each row of the framebuffer above the horizon ---
		for (int fy = 0; fy < clampedFbHorizon; ++fy) {
			// Convert this framebuffer row back to the corresponding screen-space Y pixel.
			f32 screenY = (f32)fy * invScale;

			// "p" is how many screen pixels this row is above the horizon.
			// Near the horizon p → 0, which means distance → infinity (ceiling stretches to the horizon).
			f32 p = (f32)horizon - screenY;
			if (fabsf(p) < 0.1f) continue; // Skip rows too close to the horizon (would produce huge distances).

			// The world-space distance from the camera to the ceiling point projected at this row.
			f32 rowDist = rowDistFactor / p;
			if (rowDist < 0) continue; // Negative distance shouldn't happen, but guard against it.

			// Look up the fog attenuation for this row's distance.
			f32 fogFactor = 1.0f; // 1.0 = fully visible (no fog).
			if (fog.enabled) {
				int lutIdx = (int)(rowDist * 100.0f); // LUT index at 0.01-unit precision.
				if (lutIdx < 0) lutIdx = 0;
				if (lutIdx > fogLUTMax) lutIdx = fogLUTMax;
				fogFactor = fogLUT[lutIdx];            // Exponential decay value from pre-computed table.
				if (fogFactor < fog.minVisibility) fogFactor = fog.minVisibility; // Don't go fully black.
				if (fogFactor > fog.maxVisibility) fogFactor = fog.maxVisibility; // Don't exceed max brightness.
			}
			// If this row is so far away that it's essentially invisible, stop — all rows above are farther.
			if (fogFactor < 0.01f) break;

			// Byte offset to the start of this row in the framebuffer (each pixel = 4 bytes RGBA).
			int rowOffset = fy * fbWidth * 4;

			// --- Column loop: iterate each pixel in this row ---
			for (int fx = 0; fx < fbWidth; ++fx) {
				// Convert this framebuffer column back to the corresponding screen-space X pixel.
				f32 screenX = (f32)fx * invScale;

				// Determine which raycast column block this screen pixel belongs to.
				int colIdx = (int)(screenX) / COLUMN_STEP;
				if (colIdx >= numCols) colIdx = numCols - 1; // Clamp to last column.

				const auto& col = columns[colIdx]; // Column data from the raycast pass.

				// Wall occlusion: if this screen row is at or below the wall's top edge,
				// the wall covers this pixel — skip (the wall will be drawn on top later).
				if (screenY >= col.ray.drawStart) continue;

				// Interpolate the ray direction between the column block's left and right edges.
				// t ∈ [0, 1] represents where within the COLUMN_STEP-wide block this pixel falls.
				f32 t = (screenX - (f32)(colIdx * COLUMN_STEP)) / (f32)COLUMN_STEP;
				f32 rayDirX = col.rayDirX + t * (col.rayDirX2 - col.rayDirX); // Lerp X component.
				f32 rayDirY = col.rayDirY + t * (col.rayDirY2 - col.rayDirY); // Lerp Y component.

				// Project this pixel onto the ceiling plane at the computed row distance.
				// (pos + rowDist * rayDir) gives the world-space XY coordinate on the ceiling.
				f32 worldU = pos.x + rowDist * rayDirX; // World X → used as texture U.
				f32 worldV = pos.y + rowDist * rayDirY; // World Y → used as texture V.

				// Convert world UV to texel coordinates with wrapping (handles negative values too).
				// The double-modulo (%texW + texW) % texW ensures the result is always in [0, texW-1].
				int texX = ((int)floorf(worldU * texW) % texW + texW) % texW;
				int texY = ((int)floorf(worldV * texH) % texH + texH) % texH;

					// Compute the byte index into the source texture and destination framebuffer.
				int texIdx = (texY * texW + texX) * 4; // Source texel: row * width + column, × 4 bytes.
				int fbIdx = rowOffset + fx * 4;         // Destination pixel in the framebuffer.

				// Write RGBA: multiply each colour channel by fogFactor to darken distant pixels.
				ceilFramebuffer[fbIdx + 0] = (u8)(texPixels[texIdx + 0] * fogFactor); // R
				ceilFramebuffer[fbIdx + 1] = (u8)(texPixels[texIdx + 1] * fogFactor); // G
				ceilFramebuffer[fbIdx + 2] = (u8)(texPixels[texIdx + 2] * fogFactor); // B
				ceilFramebuffer[fbIdx + 3] = 255; // A = fully opaque (ceiling is always solid).
			}
		}

		// --- GPU upload and draw ---
		// Create a transient GPU texture from our CPU pixel buffer.
		// AEGfxTextureLoadFromMemory expects raw RGBA data with the given width and height.
		AEGfxTexture* tex = AEGfxTextureLoadFromMemory(ceilFramebuffer.data(), (u32)fbWidth, (u32)fbHeight);
		if (tex && fullscreenMesh) {
			AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);                    // Draw with a texture (not solid colour).
			AEGfxTextureSet(tex, 0, 0);                                // Bind our framebuffer texture, UV offset (0,0).
			AEGfxSetBlendMode(AE_GFX_BM_BLEND);                       // Enable alpha blending (transparent pixels show through).
			AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);         // No tint — full colour passthrough.
			AEGfxSetColorToAdd(0, 0, 0, 0);                           // No additive colour.
			AEGfxSetTransparency(1.0f);                                // Full opacity for the quad itself.

			// Scale the unit quad (-0.5..0.5) to cover the entire screen.
			AEMtx33 scale, transform;
			AEMtx33Scale(&scale, (f32)screenWidth, (f32)screenHeight); // Stretch to screen dimensions.
			AEMtx33Identity(&transform);
			AEMtx33Concat(&transform, &transform, &scale);            // Combined transform = identity × scale.
			AEGfxSetTransform(transform.m);                            // Apply the transform.
			AEGfxMeshDraw(fullscreenMesh, AE_GFX_MDM_TRIANGLES);      // Draw the two-triangle fullscreen quad.
			AEGfxTextureUnload(tex);                                   // Free the transient texture immediately.
		}
	}
	// ===================================================================
	// Framebuffer-based floor rendering
	//
	// Mirrors RenderCeiling but iterates rows *below* the horizon.
	// Includes an early-out optimisation: rows near the horizon that are
	// beyond the maximum visible fog distance are skipped entirely.
	// ===================================================================
	void TileMapRenderer::RenderFloor(const Vector2& pos, f32 camZ, int horizon, int screenWidth, int screenHeight, 
		const std::vector<ColumnInfo>& columns, TileMap* map) {
		
		if (floorSprites.empty() || columns.empty()) return; // No floor sprite — nothing to draw.
		if (floorCpuTex.pixels.empty() || floorCpuTex.width == 0) return; // CPU texture not loaded.

		// Compute the internal framebuffer resolution.
		int scaledW = (int)(screenWidth * fbScale);
		int scaledH = (int)(screenHeight * fbScale);
		if (scaledW < 1) scaledW = 1;
		if (scaledH < 1) scaledH = 1;

		// Re-allocate BOTH framebuffers if the dimensions have changed.
		// Both buffers share fbWidth/fbHeight, so whichever function runs first must resize both.
		if (fbWidth != scaledW || fbHeight != scaledH) {
			fbWidth = scaledW;
			fbHeight = scaledH;
			floorFramebuffer.resize(fbWidth * fbHeight * 4, 0);
			ceilFramebuffer.resize(fbWidth * fbHeight * 4, 0);
		}

		// Clear to transparent black so unwritten pixels don't affect the scene.
		memset(floorFramebuffer.data(), 0, floorFramebuffer.size());

		// posZ = camera height above ground. For the floor, we project downward.
		f32 posZ = camZ;
		// rowDistFactor / p gives the world distance for rows below the horizon.
		f32 rowDistFactor = posZ * (f32)screenWidth / (cachedPlaneLength * 2.0f);

		// Map the horizon into framebuffer space.
		f32 fbHorizon = (f32)horizon * fbScale;
		int clampedFbHorizon = (int)fbHorizon;
		if (clampedFbHorizon < 0) clampedFbHorizon = 0;
		if (clampedFbHorizon > fbHeight) clampedFbHorizon = fbHeight;

		const f32 invScale = 1.0f / fbScale; // fb pixel → screen pixel multiplier.

		// --- Early-out: skip rows near the horizon that would be fully fogged ---
		// exp(-d / fogDist) ≈ 0.01 when d ≈ 4.6 × fogDist, so beyond that distance
		// the floor is essentially invisible. We can start iterating from the first
		// row that maps to a visible distance instead of the horizon itself.
		int startFy = clampedFbHorizon; // Default: start right at the horizon.
		if (fog.enabled && posZ > 0.001f) {
			f32 maxVisibleDist = fog.distance * 4.6f;     // Beyond this, fog ≈ 0.
			f32 minP = rowDistFactor / maxVisibleDist;    // Minimum "p" that gives a visible distance.
			int computedScreenStart = horizon + (int)ceilf(minP); // Screen row corresponding to minP.
			int computedFbStart = (int)(computedScreenStart * fbScale); // Map to framebuffer space.
			if (computedFbStart > clampedFbHorizon) {
				startFy = computedFbStart; // Skip the fully-fogged rows near the horizon.
			}
		}

		// Cache constants for the inner loop.
		const int fogLUTMax = (int)fogLUT.size() - 1;
		const int texW = floorCpuTex.width;
		const int texH = floorCpuTex.height;
		const u8* texPixels = floorCpuTex.pixels.data();
		const int numCols = (int)columns.size();

		// --- Row loop: iterate each framebuffer row below the horizon ---
		for (int fy = startFy; fy < fbHeight; ++fy) {
			f32 screenY = (f32)fy * invScale;        // Map fb row → screen row.
			f32 p = screenY - (f32)horizon;          // Pixels below the horizon (positive = below).
			if (fabsf(p) < 0.1f) continue;           // Too close to horizon → skip.
			f32 rowDist = rowDistFactor / p;          // World distance to this floor row.
			if (rowDist < 0) continue;                // Safety guard.

			// Fog attenuation for this distance.
			f32 fogFactor = 1.0f;
			if (fog.enabled) {
				int lutIdx = (int)(rowDist * 100.0f);
				if (lutIdx < 0) lutIdx = 0;
				if (lutIdx > fogLUTMax) lutIdx = fogLUTMax;
				fogFactor = fogLUT[lutIdx];
				if (fogFactor < fog.minVisibility) fogFactor = fog.minVisibility;
				if (fogFactor > fog.maxVisibility) fogFactor = fog.maxVisibility;
			}
			if (fogFactor < 0.01f) continue; // Row fully fogged — skip (but don't break; closer rows may be visible).

			int rowOffset = fy * fbWidth * 4; // Byte offset to this row in the framebuffer.

			// --- Column loop ---
			for (int fx = 0; fx < fbWidth; ++fx) {
				f32 screenX = (f32)fx * invScale;              // Map fb col → screen col.
				int colIdx = (int)(screenX) / COLUMN_STEP;     // Which column block.
				if (colIdx >= numCols) colIdx = numCols - 1;

				const auto& col = columns[colIdx];

				// Wall occlusion: if this row is above the wall's bottom edge, the wall covers it.
				if (screenY < col.ray.drawEnd) continue;

				// Interpolate ray direction within the column block (same as ceiling).
				f32 t = (screenX - (f32)(colIdx * COLUMN_STEP)) / (f32)COLUMN_STEP;
				f32 rayDirX = col.rayDirX + t * (col.rayDirX2 - col.rayDirX);
				f32 rayDirY = col.rayDirY + t * (col.rayDirY2 - col.rayDirY);

				// Project onto the floor plane.
				f32 worldU = pos.x + rowDist * rayDirX;
				f32 worldV = pos.y + rowDist * rayDirY;

				// Texture sampling with wrapping.
				int texX = ((int)floorf(worldU * texW) % texW + texW) % texW;
				int texY = ((int)floorf(worldV * texH) % texH + texH) % texH;

				int texIdx = (texY * texW + texX) * 4; // Source texel byte offset.
				int fbIdx = rowOffset + fx * 4;         // Destination byte offset.

				// Write fogged RGBA.
				floorFramebuffer[fbIdx + 0] = (u8)(texPixels[texIdx + 0] * fogFactor); // R
				floorFramebuffer[fbIdx + 1] = (u8)(texPixels[texIdx + 1] * fogFactor); // G
				floorFramebuffer[fbIdx + 2] = (u8)(texPixels[texIdx + 2] * fogFactor); // B
				floorFramebuffer[fbIdx + 3] = 255; // A = opaque.
			}
		}

		// --- GPU upload and draw (same pattern as ceiling) ---
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

	// ===================================================================
	// Wall rendering (GPU-accelerated via AEGfxTriAdd batching)
	//
	// By this point, `columns` has been sorted by (tileType, fogKey).
	// We iterate linearly and accumulate triangles into an open AEGfx mesh.
	// Whenever the tile type or fog key changes, we flush (finalise + draw)
	// the current mesh and start a new one with the new GPU state.
	// This keeps GPU state changes to a minimum (~1 draw call per unique
	// tileType+fogKey combination).
	// ===================================================================
	void TileMapRenderer::RenderWalls(int horizon, int screenHeight, const std::vector<ColumnInfo>& columns) {
		if (columns.empty()) return; // Nothing to draw.

		AEGfxTextureMode(AE_GFX_TM_AVERAGE); // Bilinear filtering for wall textures.

		// Batch state: tracks the currently-open mesh's tile type and fog level.
		int currentTileType = -1;      // -1 means "no batch started yet".
		int currentFogKey = -1;
		bool meshIsOpen = false;       // True while we're between MeshStart and MeshEnd.
		int triCount = 0;              // Triangles added to the current mesh.
		Sprite* currentSprite = nullptr; // Wall sprite for the current tile type.

		// Cache the fallback wall colours (used when no sprite is assigned).
		Color cNS = wallColorNS; // Tint for North/South-facing walls.
		Color cEW = wallColorEW; // Tint for East/West-facing walls.

		for (const auto& col : columns) {
			// --- Early-out culling ---
			// Skip columns that are entirely off-screen vertically.
			if (col.ray.drawEnd < 0 || col.ray.drawStart > screenHeight) continue;
			// Skip zero-height columns (wall exactly at the horizon or degenerate).
			if (col.screenY == col.screenY2) continue;
			// Skip columns that are too deeply fogged to be visible.
			if (col.fogFactor < 0.01f) continue;

			// --- Batch boundary check ---
			// If the tile type or fog key differs from the current batch, we need to
			// flush (draw) the current mesh and start a new one with different GPU state.
			if (meshIsOpen && (col.ray.tileType != currentTileType || col.fogKey != currentFogKey)) {
				AEGfxVertexList* mesh = AEGfxMeshEnd(); // Finalise the mesh vertex buffer.
				if (mesh) {
					if (currentSprite) {
						// Textured wall: set the wall sprite texture with fog as a colour multiplier.
						AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
						AEGfxTextureSet(currentSprite->GetTexture(), 0, 0);
						AEGfxSetBlendMode(AE_GFX_BM_BLEND);
						float drawFog = (float)currentFogKey / fog.precision; // Dequantise fog [0, 1].
						AEGfxSetColorToMultiply(drawFog, drawFog, drawFog, 1.0f); // Darken by fog.
						AEGfxSetColorToAdd(0, 0, 0, 0);
						AEGfxSetTransparency(1.0f);
					} else {
						// Solid-colour fallback (no sprite assigned for this tile type).
						AEGfxSetRenderMode(AE_GFX_RM_COLOR);
						AEGfxSetBlendMode(AE_GFX_BM_NONE);
						AEGfxSetColorToMultiply(1, 1, 1, 1);
						AEGfxSetColorToAdd(0, 0, 0, 0);
					}
					AEGfxSetTransform(identityMtx.m);              // Screen-space (no model transform needed).
					AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);     // Issue the draw call.
					AEGfxMeshFree(mesh);                            // Free the dynamic mesh.
				}
				meshIsOpen = false; // Mark that we need to start a new mesh.
			}

			// --- Start a new batch if needed ---
			if (!meshIsOpen) {
				AEGfxMeshStart();                            // Begin accumulating vertices.
				meshIsOpen = true;
				triCount = 0;
				currentTileType = col.ray.tileType;         // Record the batch's tile type.
				currentFogKey = col.fogKey;                  // Record the batch's fog level.
				currentSprite = GetWallSprite(currentTileType); // Look up the wall sprite for this type.
			}

			// --- Add this column's wall quad (2 triangles) ---
			if (currentSprite) {
				// Textured quad: vertex format is (x, y, colour, u, v).
				// wallX is the world-space U coordinate; vTop/vBottom are the V extents.
				// Triangle 1: top-left, top-right, bottom-left.
				AEGfxTriAdd(col.screenX, col.screenY2, 0xFFFFFFFF, col.ray.wallX, col.vTop,
							col.screenX2, col.screenY2, 0xFFFFFFFF, col.ray.wallX, col.vTop,
							col.screenX, col.screenY, 0xFFFFFFFF, col.ray.wallX, col.vBottom);
				// Triangle 2: bottom-left, top-right, bottom-right.
				AEGfxTriAdd(col.screenX, col.screenY, 0xFFFFFFFF, col.ray.wallX, col.vBottom,
							col.screenX2, col.screenY2, 0xFFFFFFFF, col.ray.wallX, col.vTop,
							col.screenX2, col.screenY, 0xFFFFFFFF, col.ray.wallX, col.vBottom);
			} else {
				// Solid-colour fallback: bake fog into the vertex colour directly.
				Color c = (col.ray.side == 0) ? cNS : cEW;          // Choose N/S or E/W tint.
				float f = (float)currentFogKey / fog.precision;      // Dequantise fog.
				// Pack RGBA into a single u32 (ARGB byte order expected by AEGfx).
				u32 wallColorU32 = (((u32)(c.r * f * 255)) << 24) | (((u32)(c.g * f * 255)) << 16) | (((u32)(c.b * f * 255)) << 8) | 0xFF;
				AEGfxTriAdd(col.screenX, col.screenY2, wallColorU32, 0, 0, col.screenX2, col.screenY2, wallColorU32, 0, 0, col.screenX, col.screenY, wallColorU32, 0, 0);
				AEGfxTriAdd(col.screenX, col.screenY, wallColorU32, 0, 0, col.screenX2, col.screenY2, wallColorU32, 0, 0, col.screenX2, col.screenY, wallColorU32, 0, 0);
			}
			triCount += 2; // Each column adds 2 triangles (one quad).

			// --- Mid-loop mesh overflow flush ---
			// AEGfx meshes have a triangle limit. If we exceed it, flush and restart
			// without changing the batch key so the next column continues seamlessly.
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
				AEGfxMeshStart(); // Restart accumulating into a fresh mesh.
				triCount = 0;
			}
		}

		// --- Final flush: draw whatever triangles remain in the open mesh ---
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

	// 1×1 unit quad used by DrawColumn for solid-colour fallback walls.
	// Two triangles form a square from (0,0) to (1,1) with full UVs.
	void TileMapRenderer::CreateColumnMesh() {
		AEGfxMeshStart();
		// Triangle 1: bottom-left → top-left → top-right.
		AEGfxTriAdd(0.0f, 0.0f, 0xFFFFFFFF, 0.0f, 1.0f, 0.0f, 1.0f, 0xFFFFFFFF, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 1.0f, 0.0f);
		// Triangle 2: bottom-left → top-right → bottom-right.
		AEGfxTriAdd(0.0f, 0.0f, 0xFFFFFFFF, 0.0f, 1.0f, 1.0f, 1.0f, 0xFFFFFFFF, 1.0f, 0.0f, 1.0f, 0.0f, 0xFFFFFFFF, 1.0f, 1.0f);
		columnMesh = AEGfxMeshEnd();
	}

	// Draws a single solid-colour column strip (fallback when no wall sprite assigned).
	// Positions the 1×1 column mesh at (x, y) with height h using scale + translate matrices.
	void TileMapRenderer::DrawColumn(f32 x, f32 y, f32 h, const Color& color, int sw, int sh) {
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);                        // No texture — flat colour.
		AEGfxSetBlendMode(AE_GFX_BM_NONE);                          // Opaque draw.
		AEGfxSetColorToMultiply(color.r, color.g, color.b, color.a); // Tint with the wall colour.
		AEGfxSetColorToAdd(0, 0, 0, 0);                             // No additive colour.

		AEMtx33 scale, trans, result;
		AEMtx33Scale(&scale, 1.0f, h);              // Scale the unit quad to the wall's height.
		f32 screenX = x - sw / 2.0f;                // Convert pixel-X to AE centred coords.
		f32 screenY = sh / 2.0f - y - h;             // Convert pixel-Y to AE centred coords (Y flipped).
		AEMtx33Trans(&trans, screenX, screenY);      // Translate to the correct screen position.
		AEMtx33Concat(&result, &trans, &scale);      // Combine: first scale, then translate.
		AEGfxSetTransform(result.m);                 // Apply the combined matrix.
		AEGfxMeshDraw(columnMesh, AE_GFX_MDM_TRIANGLES); // Draw the column.
	}


	// Pre-computes fogLUT[i] = exp(-i * 0.01 / fogDistance) for i in [0, 9999].
	// This avoids calling expf() per-pixel during rendering.
	// fogLUT covers world distances 0–100 at 0.01-unit precision (10 000 entries).
	void TileMapRenderer::UpdateFogLUT(f32 fogDistance) {
		fogLUT.resize(10000);                           // 10 000 entries = distances 0..99.99.
		for (size_t i = 0; i < fogLUT.size(); ++i) {
			f32 dist = (f32)i * 0.01f;                  // Convert index to world distance.
			fogLUT[i] = expf(-dist / fogDistance);       // Exponential decay: 1 at dist=0, →0 far away.
		}
		cachedFogDistance = fogDistance;                  // Cache the distance so we know when to rebuild.
	}

} // namespace Engine

