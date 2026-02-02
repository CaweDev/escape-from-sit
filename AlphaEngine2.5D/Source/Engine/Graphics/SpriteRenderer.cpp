/**
 * @file SpriteRenderer.cpp
 * @brief Implementation of SpriteRenderer class
 */
#include "SpriteRenderer.hpp"
#include "ZBuffer.hpp"
#include "../Core/Application.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

namespace Engine {
	SpriteRenderer::SpriteRenderer()
	{
	}

	void SpriteRenderer::SetSprite(std::shared_ptr<Sprite> newSprite) {

		//sprite = newSprite.get();
	}

	void SpriteRenderer::SetSprite(Sprite* newSprite)
	{
		sprite = newSprite;
	}

	void SpriteRenderer::Render() {
		if (!enabled || !transform) return;
		if (!sprite || !sprite->IsReady()) return;

		if (billboard && !Camera::main) {
			static bool cameraWarned = false;
			if (!cameraWarned) {
				printf("[Warning] SpriteRenderer: No Camera::main found for billboard rendering\n");
				cameraWarned = true;
			}
			return;
		}

		bool hasTexture = sprite->IsLoaded();

		// Set render mode based on whether we have a texture
		if (hasTexture) {
			AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
			AEGfxTextureSet(sprite->GetTexture(), 0, 0);
		}
		else {
			AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		}

		// Set blend mode
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);

		// Set color multiply (tint)
		AEGfxSetColorToMultiply(color.r, color.g, color.b, color.a);

		// Set global transparency
		AEGfxSetTransparency(color.a);

		// Set color add (highlight effect when targeted)
		if (highlighted) {
			AEGfxSetColorToAdd(highlightColor.r, highlightColor.g, highlightColor.b, highlightColor.a);
		} else {
			AEGfxSetColorToAdd(0, 0, 0, 0);
		}

		AEMtx33 result;

		if (billboard && Camera::main) {
			// 2.5D Raycast Projection
			int sw = Application::GetWindowWidth();
			int sh = Application::GetWindowHeight();

			ProjectionResult proj = ProjectionHelper::ProjectToScreen(
				transform->position,
				Camera::main->transform->position,
				Camera::main->transform->rotation,
				Camera::main->fov,
				sw,
				sh
			);

			if (!proj.isVisible) return;

			// Calculate sprite screen size
			// spriteHeight = proj.scale (pixels per unit) * localScale.y
			// spriteWidth = proj.scale (pixels per unit) * localScale.x
			f32 spriteHeight = proj.scale * transform->localScale.y;
			f32 spriteWidth = proj.scale * transform->localScale.x;

			// Calculate world width for per-column depth
			f32 worldWidth = transform->localScale.x;
			
			// Camera vectors for per-column depth calculation
			f32 camRotRad = Camera::main->transform->rotation * 3.14159f / 180.0f;
			f32 camForwardX = cosf(camRotRad);
			f32 camForwardY = sinf(camRotRad);
			f32 camRightX = sinf(camRotRad);
			f32 camRightY = -cosf(camRotRad);

			// Center the sprite horizontally around proj.screenX
			f32 startX = proj.screenX - spriteWidth / 2.0f;
			f32 endX = proj.screenX + spriteWidth / 2.0f; 

			// Screen clipping bounds for the loop
			int loopStart = static_cast<int>(floorf(startX));
			int loopEnd = static_cast<int>(floorf(endX));

			// Further clip to actual screen visible area
			int screenStart = (loopStart < 0) ? 0 : loopStart;
			int screenEnd = (loopEnd >= sw) ? sw - 1 : loopEnd;

			if (screenStart > screenEnd) return;

			// FOV for per-column depth
			f32 fovRad = Camera::main->fov * 3.14159f / 180.0f;
			f32 tanHalfFov = tanf(fovRad / 2.0f);

			// Batch visible strips into a single dynamic mesh
			AEGfxMeshStart();
			bool inStrip = false;
			int stripStart = 0;

			// Position sprite so its bottom (feet) is at proj.screenY, extending upward
			// proj.screenY is in screen coordinates (0 at top, increases downward)
			// Convert to AE coordinates (0 at center, positive up)
			// When camera is higher, proj.screenY increases (lower on screen), yBottom decreases
			f32 yBottom = (sh / 2.0f) - proj.screenY;
			f32 yTop = yBottom + spriteHeight;

			for (int x = screenStart; x <= screenEnd; ++x) {
				// Calculate per-column depth:
				// 1. Find UV (0-1) across sprite width for this column
				f32 u = (static_cast<f32>(x) - startX) / spriteWidth;
				
				// 2. Calculate world position of this column on the billboard
				f32 offsetFromCenter = (u - 0.5f) * worldWidth;
				f32 columnWorldX = transform->position.x + camRightX * offsetFromCenter;
				f32 columnWorldY = transform->position.y + camRightY * offsetFromCenter;
				
				// 3. Calculate perpendicular distance for this column
				f32 dx = columnWorldX - Camera::main->transform->position.x;
				f32 dy = columnWorldY - Camera::main->transform->position.y;
				f32 columnPerpDist = dx * camForwardX + dy * camForwardY;
				
				// 4. Compare against Z-buffer
				bool visible = columnPerpDist < ZBuffer::GetDepth(x);
				
				if (visible && !inStrip) {
					stripStart = x;
					inStrip = true;
				}
				else if (!visible && inStrip) {
					// Finish current strip
					AddStripToMesh(stripStart, x - 1, startX, spriteWidth, yTop, yBottom);
					inStrip = false;
				}
			}
			if (inStrip) {
				AddStripToMesh(stripStart, screenEnd, startX, spriteWidth, yTop, yBottom);
			}

			AEGfxVertexList* dynamicMesh = AEGfxMeshEnd();
			if (dynamicMesh) {
				AEMtx33 identity;
				AEMtx33Identity(&identity);
				AEGfxSetTransform(identity.m); // We use screen space coords directly
				AEGfxMeshDraw(dynamicMesh, AE_GFX_MDM_TRIANGLES);
				AEGfxMeshFree(dynamicMesh);
			}
		}
		else {
			// Standard 2D rendering logic (existing)
			f32 scaleX = transform->localScale.x;
			f32 scaleY = transform->localScale.y;

			if (flipX) scaleX = -scaleX;
			if (flipY) scaleY = -scaleY;

			AEMtx33 scale, rot, trans;
			AEMtx33Scale(&scale, scaleX, scaleY);
			AEMtx33RotDeg(&rot, transform->rotation);
			AEMtx33Trans(&trans, transform->position.x, transform->position.y);

			AEMtx33Concat(&result, &rot, &scale);
			AEMtx33Concat(&result, &trans, &result);

			if (transform->parent) {
				AEMtx33 parentMat = transform->parent->GetMatrix();
				AEMtx33Concat(&result, &parentMat, &result);
			}

			AEGfxSetTransform(result.m);
			AEGfxMeshDraw(sprite->GetMesh(), AE_GFX_MDM_TRIANGLES);
		}
	}

	void SpriteRenderer::AddStripToMesh(int x1, int x2, f32 spriteStart, f32 spriteWidth, f32 yTop, f32 yBottom) {
		int sw = Application::GetWindowWidth();
		int sh = Application::GetWindowHeight();

		// Convert screen coords to AE coords (center is 0,0)
		f32 sx1 = static_cast<f32>(x1) - sw / 2.0f;
		f32 sx2 = static_cast<f32>(x2 + 1) - sw / 2.0f;

		// Clip the quad geometry to the actual sprite bounds to avoid rendering junk at partial pixel edges
		f32 spriteStartAE = spriteStart - sw / 2.0f;
		f32 spriteEndAE = spriteStartAE + spriteWidth;

		if (sx1 < spriteStartAE) sx1 = spriteStartAE;
		if (sx2 > spriteEndAE) sx2 = spriteEndAE;

		// Calculate UVs using clipped geometry to stay within [0, 1]
		f32 u1 = (sx1 - spriteStartAE) / spriteWidth;
		f32 u2 = (sx2 - spriteStartAE) / spriteWidth;

		if (flipX) {
			u1 = 1.0f - u1;
			u2 = 1.0f - u2;
		}

		f32 vTop = 0.0f;
		f32 vBottom = 1.0f;

		if (flipY) {
			vTop = 1.0f;
			vBottom = 0.0f;
		}

		// Use white for vertices since tint is handled by global multiply
		u32 vertexColor = 0xFFFFFFFF;

		// Triangle 1
		AEGfxTriAdd(
			sx1, yTop, vertexColor, u1, vTop,
			sx2, yTop, vertexColor, u2, vTop,
			sx1, yBottom, vertexColor, u1, vBottom
		);
		// Triangle 2
		AEGfxTriAdd(
			sx1, yBottom, vertexColor, u1, vBottom,
			sx2, yTop, vertexColor, u2, vTop,
			sx2, yBottom, vertexColor, u2, vBottom
		);
	}

	f32 SpriteRenderer::GetSortingKey() const {
		if (!transform) return static_cast<f32>(sortingOrder);

		if (Camera::main && Camera::main->transform) {
			// Calculate planar distance (dot product along camera forward vector)
			// This prevents popping when rotating the camera
			f32 camRotRad = Camera::main->transform->rotation * 3.14159f / 180.0f;
			f32 camForwardX = cosf(camRotRad);
			f32 camForwardY = sinf(camRotRad);

			f32 dx = transform->position.x - Camera::main->transform->position.x;
			f32 dy = transform->position.y - Camera::main->transform->position.y;

			// Distance along the camera's view direction
			f32 dist = dx * camForwardX + dy * camForwardY;

			return dist;
		}

		// Fallback if no camera
		return transform->position.z * 10000.0f + static_cast<f32>(sortingOrder);
	}

} // namespace Engine
