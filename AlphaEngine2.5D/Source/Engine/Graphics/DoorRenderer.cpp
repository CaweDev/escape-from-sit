/**
 * @file DoorRenderer.cpp
 * @brief Implementation of DoorRenderer component - renders doors as axis-aligned wall segments
 */
#include "DoorRenderer.hpp"
#include "Camera.hpp"
#include "ZBuffer.hpp"
#include "RendererSettings.hpp"
#include "../TileMap/Door.hpp"
#include "../Core/GameObject.hpp"
#include "../Core/Application.hpp"
#include "../Core/ConfigManager.hpp"
#include "../Math/Mathf.hpp"
#include <cmath>
#include <cstdio>

namespace Engine {

DoorRenderer::DoorRenderer() : doorMesh(nullptr) {}

DoorRenderer::~DoorRenderer() {
    if (doorMesh) {
        AEGfxMeshFree(doorMesh);
        doorMesh = nullptr;
    }
}

// Config defaults, populated by DoorRenderer::Initialize()
static DoorRendererConfig s_doorRendererConfig;

void DoorRenderer::Initialize(const std::string& configPath)
{
    if (!ConfigManager::ReadConfig(configPath, s_doorRendererConfig))
    {
        std::printf("[Warning] DoorRenderer: Failed to read config '%s'. Using built-in defaults.\n",
                    configPath.c_str());
    }
    // Note: existing instances are unaffected; defaults apply to instances
    // whose members are set from s_doorRendererConfig at construction or spawn time.
}

/*!
 * \brief [AUTO] CacheReferences
 */
void DoorRenderer::CacheReferences() {
    if (!door && gameObject) {
        door = gameObject->GetComponent<Door>();
    }
}

/*!
 * \brief [AUTO] CreateMesh
 */
void DoorRenderer::CreateMesh() {
    // Not used - we build columns dynamically
}

/*!
 * \brief [AUTO] CalculatePerpDistance
 * \return 
 */
/*!
 * \brief [AUTO] CalculatePerpDistance
 * \return 
 */
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

/*!
 * \brief [AUTO] GetSortingKey
 * \return 
 */
/*!
 * \brief [AUTO] GetSortingKey
 * \return 
 */
f32 DoorRenderer::GetSortingKey() const {
    return CalculatePerpDistance();
}

/*!
 * \brief [AUTO] Render
 */
void DoorRenderer::Render() {
    if (!enabled) return;
    
    CacheReferences();
    if (!door) return;
    
    // NOTE: Don't early-return for fully open doors anymore.
    // Horizontally sliding doors may still be visible from certain angles.
    
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
    
    // Door segment in world space - apply horizontal slide based on openAmount
    f32 x1, y1, x2, y2;
    f32 slideOffset = door->openAmount; // 0 = closed, 1 = fully open (slid 1 unit)
    
    if (door->facing == DoorFacing::NorthSouth) {
        // Runs East-West (constant Y), blocks North-South movement
        // Slides in +X direction when opening
        x1 = static_cast<f32>(door->tileX) + slideOffset;
        y1 = static_cast<f32>(door->tileY) + 0.5f;
        x2 = x1 + 1.0f;
        y2 = y1;
    } else {
        // Runs North-South (constant X), blocks East-West movement
        // Slides in +Y direction when opening
        x1 = static_cast<f32>(door->tileX) + 0.5f;
        y1 = static_cast<f32>(door->tileY) + slideOffset;
        x2 = x1;
        y2 = y1 + 1.0f;
    }

    bool meshStarted = false;
    int triCount = 0;
    const int MAX_TRIS = 1000;

    AEGfxSetCamPosition(0.0f, 0.0f);

    // Pre-calculate average fog factor for the entire door mesh
    f32 centerDist = CalculatePerpDistance();
    f32 meshFog = 1.0f;
    if (Camera::main && Camera::main->fogEnabled && !ignoreFog) {
        meshFog = expf(-centerDist / Camera::main->fogDistance);
        if (meshFog < Camera::main->fogMinVisibility) meshFog = Camera::main->fogMinVisibility;
        if (meshFog > Camera::main->fogMaxVisibility) meshFog = Camera::main->fogMaxVisibility;
    }

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
            if (fabsf(rayDirY) > RendererSettings::doorRayDirThreshold) {
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
            if (fabsf(rayDirX) > RendererSettings::doorRayDirThreshold) {
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
            
            if (perpDist < RendererSettings::perpWallDistClamp) perpDist = RendererSettings::perpWallDistClamp;

            // Z-Buffer check
            if (x >= 0 && x < ZBuffer::GetWidth()) {
                if (perpDist > ZBuffer::GetDepth(x) + RendererSettings::doorZBufferBias) {
                    continue; // Behind something
                }
            }

            // Calculate wall drawing parameters
            f32 wallHeight = (f32)screenWidth / (perpDist * planeLength * 2.0f);
            f32 heightOffset = (camPos.z - 0.5f) * ((f32)screenWidth / (perpDist * planeLength * 2.0f));
            f32 drawStart = (f32)horizon - (wallHeight / 2.0f) + heightOffset;
            f32 drawEnd = (f32)horizon + (wallHeight / 2.0f) + heightOffset;

            // Allow the GPU to natively clip off-screen coordinates rather than manually clamping,
            // otherwise the texture UV map squashes into the visible area.
            if (drawStart >= drawEnd) continue;

            // Render this column
            if (!meshStarted) {
                AEGfxMeshStart();
                meshStarted = true;
                triCount = 0;
            }

            f32 aeX = (f32)x - screenWidth / 2.0f;
            f32 aeX2 = (f32)(x + 1) - screenWidth / 2.0f;
            f32 aeY_top = screenHeight / 2.0f - drawStart;
            f32 aeY_bot = screenHeight / 2.0f - drawEnd;

            // Camera-based exponential fog (consistent with TileMapRenderer)
            f32 fog = 1.0f;
            if (Camera::main && Camera::main->fogEnabled && !ignoreFog) {
                fog = expf(-perpDist / Camera::main->fogDistance);
                if (fog < Camera::main->fogMinVisibility) fog = Camera::main->fogMinVisibility;
                if (fog > Camera::main->fogMaxVisibility) fog = Camera::main->fogMaxVisibility;
            }

            Color c = (door->facing == DoorFacing::NorthSouth) ? colorNS : colorEW;
            if (highlighted) {
                c.r += highlightColor.r;
                c.g += highlightColor.g;
                c.b += highlightColor.b;
            }
            u32 color = ((u32)(Mathf::Clamp01(c.r * fog) * 255) << 24) | 
                        ((u32)(Mathf::Clamp01(c.g * fog) * 255) << 16) | 
                        ((u32)(Mathf::Clamp01(c.b * fog) * 255) << 8) | 0xFF;

            // UV: wallU is horizontal texture coordinate, V is 0-1 for vertical
            AEGfxTriAdd(aeX, aeY_top, color, wallU, 0.0f,
                        aeX2, aeY_top, color, wallU, 0.0f,
                        aeX, aeY_bot, color, wallU, 1.0f);
            AEGfxTriAdd(aeX, aeY_bot, color, wallU, 1.0f,
                        aeX2, aeY_top, color, wallU, 0.0f,
                        aeX2, aeY_bot, color, wallU, 1.0f);
            triCount += 2;

            if (triCount >= MAX_TRIS) {
                AEGfxVertexList* mesh = AEGfxMeshEnd();
                if (mesh) {
                    RenderMesh(mesh, meshFog);
                    AEGfxMeshFree(mesh);
                }
                meshStarted = false;
            }
        }
    }

    if (meshStarted) {
        AEGfxVertexList* mesh = AEGfxMeshEnd();
        if (mesh) {
            RenderMesh(mesh, meshFog);
            AEGfxMeshFree(mesh);
        }
    }
}

/*!
 * \brief [AUTO] RenderMesh
 * \param mesh [in]
 * \param fogFactor [in]
 */
/*!
 * \brief [AUTO] RenderMesh
 * \param mesh [in]
 * \param fogFactor [in]
 */
void DoorRenderer::RenderMesh(AEGfxVertexList* mesh, f32 fogFactor) {
    if (sprite && sprite->IsLoaded()) {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(sprite->GetTexture(), 0, 0);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    } else {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_NONE);
    }
    // Apply fog darkening uniformly to the mesh via texture multiply color
    AEGfxSetColorToMultiply(fogFactor, fogFactor, fogFactor, 1.0f);
    
    if (highlighted) {
        AEGfxSetColorToAdd(highlightColor.r, highlightColor.g, highlightColor.b, highlightColor.a);
    } else {
        AEGfxSetColorToAdd(0, 0, 0, 0);
    }
    
    AEMtx33 identity;
    AEMtx33Identity(&identity);
    AEGfxSetTransform(identity.m);
    
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

} // namespace Engine
