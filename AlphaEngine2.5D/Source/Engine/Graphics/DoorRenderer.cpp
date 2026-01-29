/**
 * @file DoorRenderer.cpp
 * @brief Implementation of DoorRenderer component - renders doors as axis-aligned wall segments
 */
#include "DoorRenderer.hpp"
#include "Camera.hpp"
#include "ZBuffer.hpp"
#include "../TileMap/Door.hpp"
#include "../Core/GameObject.hpp"
#include "../Core/Application.hpp"
#include "../Math/Mathf.hpp"
#include <cmath>

namespace Engine {

DoorRenderer::DoorRenderer() : doorMesh(nullptr) {}

DoorRenderer::~DoorRenderer() {
    if (doorMesh) {
        AEGfxMeshFree(doorMesh);
        doorMesh = nullptr;
    }
}

void DoorRenderer::CacheReferences() {
    if (!door && gameObject) {
        door = gameObject->GetComponent<Door>();
    }
}

void DoorRenderer::CreateMesh() {
    // Not used - we build columns dynamically
}

f32 DoorRenderer::CalculatePerpDistance() const {
    if (!door || !Camera::main || !Camera::main->transform) {
        return 1e30f;
    }
    
    // For sorting, we can use the center position distance projected onto camera forward
    Vector2 doorPos = door->GetWorldPosition();
    Vector3 camPos = Camera::main->transform->position;
    f32 camAngle = Camera::main->transform->rotation * Mathf::Deg2Rad;
    
    f32 dx = doorPos.x - camPos.x;
    f32 dy = doorPos.y - camPos.y;
    
    f32 dirX = cosf(camAngle);
    f32 dirY = sinf(camAngle);
    
    f32 perpDist = dx * dirX + dy * dirY;
    if (perpDist < 0.1f) perpDist = 0.1f;
    
    return perpDist;
}

f32 DoorRenderer::GetSortingKey() const {
    return CalculatePerpDistance();
}

void DoorRenderer::Render() {
    if (!enabled) return;
    
    CacheReferences();
    if (!door) return;
    
    // Don't render if fully open
    if (door->openAmount >= 1.0f) return;
    
    if (!Camera::main || !Camera::main->transform) return;
    
    int screenWidth = Application::GetWindowWidth();
    int screenHeight = Application::GetWindowHeight();
    if (screenWidth <= 0 || screenHeight <= 0) return;
    
    Vector3 camPos = Camera::main->transform->position;
    f32 camAngle = Camera::main->transform->rotation * Mathf::Deg2Rad;
    f32 pitchOffset = Camera::main->pitch;
    int horizon = screenHeight / 2 + static_cast<int>(pitchOffset);
    f32 fovRad = fov * Mathf::Deg2Rad;
    f32 planeLength = tanf(fovRad / 2.0f);
    
    f32 dirX = cosf(camAngle);
    f32 dirY = sinf(camAngle);
    f32 planeX = dirY * planeLength;
    f32 planeY = -dirX * planeLength;
    
    // Door segment in world space
    f32 x1, y1, x2, y2;
    if (door->facing == DoorFacing::NorthSouth) {
        // Runs East-West (constant Y), blocks North-South movement
        x1 = static_cast<f32>(door->tileX);
        y1 = static_cast<f32>(door->tileY) + 0.5f;
        x2 = x1 + 1.0f;
        y2 = y1;
    } else {
        // Runs North-South (constant X), blocks East-West movement
        x1 = static_cast<f32>(door->tileX) + 0.5f;
        y1 = static_cast<f32>(door->tileY);
        x2 = x1;
        y2 = y1 + 1.0f;
    }

    bool meshStarted = false;
    int triCount = 0;
    const int MAX_TRIS = 1000;

    AEGfxSetCamPosition(0.0f, 0.0f);

    // Render column by column like the TileMapRenderer
    for (int x = 0; x < screenWidth; ++x) {
        f32 cameraX = 2.0f * x / (f32)screenWidth - 1.0f;
        f32 rayDirX = dirX + planeX * cameraX;
        f32 rayDirY = dirY + planeY * cameraX;

        f32 tHit = -1.0f;
        f32 wallU = 0.0f;

        // Intersect ray with door segment
        if (door->facing == DoorFacing::NorthSouth) {
            // y1 = camPos.y + t * rayDirY => t = (y1 - camPos.y) / rayDirY
            if (fabsf(rayDirY) > 0.0001f) {
                tHit = (y1 - camPos.y) / rayDirY;
                if (tHit > 0) {
                    f32 hitX = camPos.x + tHit * rayDirX;
                    if (hitX >= x1 && hitX <= x2) {
                        wallU = hitX - x1;
                    } else {
                        tHit = -1.0f;
                    }
                }
            }
        } else {
            // x1 = camPos.x + t * rayDirX => t = (x1 - camPos.x) / rayDirX
            if (fabsf(rayDirX) > 0.0001f) {
                tHit = (x1 - camPos.x) / rayDirX;
                if (tHit > 0) {
                    f32 hitY = camPos.y + tHit * rayDirY;
                    if (hitY >= y1 && hitY <= y2) {
                        wallU = hitY - y1;
                    } else {
                        tHit = -1.0f;
                    }
                }
            }
        }

        if (tHit > 0) {
            // Ray hit the door!
            // Perp distance is simply tHit because the ray direction is (dir + plane * cameraX)
            // and (rayDir dot dir) = 1, so tHit's projection onto the camera direction is exactly tHit.
            f32 perpDist = tHit;
            
            if (perpDist < 0.1f) perpDist = 0.1f;

            // Z-Buffer check
            if (x >= 0 && x < ZBuffer::GetWidth()) {
                if (perpDist > ZBuffer::GetDepth(x) + 0.01f) {
                    continue; // Behind something
                }
            }

            // Calculate wall drawing parameters
            f32 wallHeight = (f32)screenWidth / (perpDist * planeLength * 2.0f);
            f32 heightOffset = (camPos.z - 0.5f) * ((f32)screenWidth / (perpDist * planeLength * 2.0f));
            f32 drawStart = (f32)horizon - (wallHeight / 2.0f) + heightOffset;
            f32 drawEnd = (f32)horizon + (wallHeight / 2.0f) + heightOffset;

            // Apply door animation (slides up)
            f32 visibleHeight = wallHeight * (1.0f - door->openAmount);
            f32 actualStart = drawEnd - visibleHeight;
            f32 actualEnd = drawEnd;

            // Clamp Y
            if (actualStart < 0) actualStart = 0;
            if (actualEnd >= screenHeight) actualEnd = (f32)screenHeight - 1;

            if (actualStart >= actualEnd) continue;

            // Render this column
            if (!meshStarted) {
                AEGfxMeshStart();
                meshStarted = true;
                triCount = 0;
            }

            f32 aeX = (f32)x - screenWidth / 2.0f;
            f32 aeX2 = (f32)(x + 1) - screenWidth / 2.0f;
            f32 aeY_top = screenHeight / 2.0f - actualStart;
            f32 aeY_bot = screenHeight / 2.0f - actualEnd;

            f32 fog = 1.0f - (perpDist / 20.0f);
            if (fog < 0.2f) fog = 0.2f;

            Color c = (door->facing == DoorFacing::NorthSouth) ? colorNS : colorEW;
            u32 color = ((u32)(c.r * fog * 255) << 24) | ((u32)(c.g * fog * 255) << 16) | ((u32)(c.b * fog * 255) << 8) | 0xFF;

            AEGfxTriAdd(aeX, aeY_top, color, wallU, door->openAmount,
                        aeX2, aeY_top, color, wallU, door->openAmount,
                        aeX, aeY_bot, color, wallU, 1.0f);
            AEGfxTriAdd(aeX, aeY_bot, color, wallU, 1.0f,
                        aeX2, aeY_top, color, wallU, door->openAmount,
                        aeX2, aeY_bot, color, wallU, 1.0f);
            triCount += 2;

            if (triCount >= MAX_TRIS) {
                AEGfxVertexList* mesh = AEGfxMeshEnd();
                if (mesh) {
                    RenderMesh(mesh);
                    AEGfxMeshFree(mesh);
                }
                meshStarted = false;
            }
        }
    }

    if (meshStarted) {
        AEGfxVertexList* mesh = AEGfxMeshEnd();
        if (mesh) {
            RenderMesh(mesh);
            AEGfxMeshFree(mesh);
        }
    }
}

void DoorRenderer::RenderMesh(AEGfxVertexList* mesh) {
    if (sprite && sprite->IsLoaded()) {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(sprite->GetTexture(), 0, 0);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    } else {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_NONE);
    }
    
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    
    AEMtx33 identity;
    AEMtx33Identity(&identity);
    AEGfxSetTransform(identity.m);
    
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

} // namespace Engine
