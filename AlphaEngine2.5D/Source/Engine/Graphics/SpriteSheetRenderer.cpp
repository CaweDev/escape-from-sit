/**
 * @file SpriteSheetRenderer.cpp
 * @brief Implementation of SpriteSheetRenderer class
 */
#include "SpriteSheetRenderer.hpp"
#include "Engine/Core/Time.hpp"
#include "ZBuffer.hpp"
#include "../Math/Mathf.hpp"
#include <cmath>
#include <algorithm>

namespace Engine {

const std::string SpriteSheetRenderer::s_emptyString;

SpriteSheetRenderer::SpriteSheetRenderer() {}

SpriteSheetRenderer::~SpriteSheetRenderer() {
    FreeMesh();
}



void SpriteSheetRenderer::SetSpriteSheet(SpriteSheet* sheet) {
    spriteSheet = sheet;
    currentClip = nullptr;
    currentFrame = 0;
    elapsedTime = 0.0f;
    playing = false;

    FreeMesh();
    if (spriteSheet && spriteSheet->IsReady()) {
        CreateSpritesheetMesh();
    }
}



void SpriteSheetRenderer::Play(const std::string& animationName) {
    if (!spriteSheet) return;

    const AnimationClip* clip = spriteSheet->GetAnimation(animationName);
    if (!clip) {
        printf("[Warning] SpriteSheetRenderer: Animation '%s' not found\n", animationName.c_str());
        return;
    }

    currentClip = clip;
    currentFrame = 0;
    elapsedTime = 0.0f;
    playing = true;
}


void SpriteSheetRenderer::Stop() {
    currentFrame = 0;
    elapsedTime = 0.0f;
    playing = false;
}


void SpriteSheetRenderer::Pause() {
    playing = false;
}


void SpriteSheetRenderer::Resume() {
    if (currentClip) {
        playing = true;
    }
}



const std::string& SpriteSheetRenderer::GetCurrentAnimation() const {
    if (currentClip) {
        return currentClip->name;
    }
    return s_emptyString;
}



void SpriteSheetRenderer::SetFrame(int frame) {
    if (currentClip && frame >= 0 && frame < currentClip->frameCount) {
        currentFrame = frame;
        elapsedTime = 0.0f;
    }
}


void SpriteSheetRenderer::CreateSpritesheetMesh() {
    FreeMesh();
    if (!spriteSheet) return;

    int c = spriteSheet->GetCols();
    int r = spriteSheet->GetRows();

    f32 cellU = 1.0f / static_cast<f32>(c);
    f32 cellV = 1.0f / static_cast<f32>(r);

    // Build a 1x1 quad with UVs covering exactly one cell (top-left cell)
    // UV offset will be applied via AEGfxTextureSet to select the correct frame
    AEGfxMeshStart();

    // Triangle 1: bottom-left, top-left, top-right
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, cellV,   // bottom-left
        -0.5f,  0.5f, 0xFFFFFFFF, 0.0f, 0.0f,     // top-left
         0.5f,  0.5f, 0xFFFFFFFF, cellU, 0.0f      // top-right
    );

    // Triangle 2: bottom-left, top-right, bottom-right
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, cellV,    // bottom-left
         0.5f,  0.5f, 0xFFFFFFFF, cellU, 0.0f,     // top-right
         0.5f, -0.5f, 0xFFFFFFFF, cellU, cellV      // bottom-right
    );

    mesh = AEGfxMeshEnd();
}


void SpriteSheetRenderer::FreeMesh() {
    if (mesh) {
        AEGfxMeshFree(mesh);
        mesh = nullptr;
    }
}


void SpriteSheetRenderer::Render() {
    if (!enabled || !transform) return;
    if (!spriteSheet || !spriteSheet->IsReady() || !mesh) return;
    if (!currentClip) return;

    // --- Advance animation ---
    if (playing && currentClip->frameRate > 0.0f) {
        elapsedTime += Time::unscaledDeltaTime * playbackSpeed;
        float frameDuration = 1.0f / currentClip->frameRate;

        while (elapsedTime >= frameDuration) {
            elapsedTime -= frameDuration;
            currentFrame++;

            if (currentFrame >= currentClip->frameCount) {
                if (currentClip->loop) {
                    currentFrame = 0;
                } else {
                    currentFrame = currentClip->frameCount - 1;
                    playing = false;
                    break;
                }
            }
        }
    }

    // --- Compute UV offset for current frame ---
    int absFrame = currentClip->startFrame + currentFrame;
    int c = spriteSheet->GetCols();
    int r = spriteSheet->GetRows();

    f32 cellU = 1.0f / static_cast<f32>(c);
    f32 cellV = 1.0f / static_cast<f32>(r);

    f32 offsetX = static_cast<f32>(absFrame % c) * cellU;
    f32 offsetY = static_cast<f32>(absFrame / c) * cellV;

    // --- Billboard check ---
    if (billboard && !Camera::main) {
        static bool cameraWarned = false;
        if (!cameraWarned) {
            printf("[Warning] SpriteSheetRenderer: No Camera::main found for billboard rendering\n");
            cameraWarned = true;
        }
        return;
    }

    AEGfxTexture* tex = spriteSheet->GetTexture()->GetNativeTexture();

    // Set render mode
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    // Set blend mode
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    // Set color multiply (tint)
    AEGfxSetColorToMultiply(color.r, color.g, color.b, color.a);

    // Set global transparency
    AEGfxSetTransparency(color.a);

    // Set color add (highlight effect)
    if (highlighted) {
        AEGfxSetColorToAdd(highlightColor.r, highlightColor.g, highlightColor.b, highlightColor.a);
    } else {
        AEGfxSetColorToAdd(0, 0, 0, 0);
    }

    if (billboard && Camera::main) {
        // Billboard strips bake absolute UVs into vertices, so no texture offset
        AEGfxTextureSet(tex, 0.0f, 0.0f);

        // ---- 2.5D Billboard Rendering (mirrors SpriteRenderer) ----
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

        f32 spriteHeight = proj.scale * transform->localScale.y;
        f32 spriteWidth = proj.scale * transform->localScale.x;
        f32 worldWidth = transform->localScale.x;

        // Camera vectors for per-column depth
        f32 camRotRad = Camera::main->transform->rotation * Mathf::Deg2Rad;
        f32 camForwardX = cosf(camRotRad);
        f32 camForwardY = sinf(camRotRad);
        f32 camRightX = sinf(camRotRad);
        f32 camRightY = -cosf(camRotRad);

        f32 startX = proj.screenX - spriteWidth / 2.0f;
        f32 endX = proj.screenX + spriteWidth / 2.0f;

        int loopStart = static_cast<int>(floorf(startX));
        int loopEnd = static_cast<int>(floorf(endX));

        int screenStart = (loopStart < 0) ? 0 : loopStart;
        int screenEnd = (loopEnd >= sw) ? sw - 1 : loopEnd;

        if (screenStart > screenEnd) return;

        AEGfxMeshStart();
        bool inStrip = false;
        int stripStart = 0;

        f32 yBottom = (sh / 2.0f) - proj.screenY;
        f32 yTop = yBottom + spriteHeight;

        for (int x = screenStart; x <= screenEnd; ++x) {
            f32 u = (static_cast<f32>(x) - startX) / spriteWidth;

            f32 offsetFromCenter = (u - 0.5f) * worldWidth;
            f32 columnWorldX = transform->position.x + camRightX * offsetFromCenter;
            f32 columnWorldY = transform->position.y + camRightY * offsetFromCenter;

            f32 dx = columnWorldX - Camera::main->transform->position.x;
            f32 dy = columnWorldY - Camera::main->transform->position.y;
            f32 columnPerpDist = dx * camForwardX + dy * camForwardY;

            bool visible = columnPerpDist < ZBuffer::GetDepth(x);

            if (visible && !inStrip) {
                stripStart = x;
                inStrip = true;
            }
            else if (!visible && inStrip) {
                AddStripToMesh(stripStart, x - 1, startX, spriteWidth, yTop, yBottom,
                               offsetX, offsetY, cellU, cellV);
                inStrip = false;
            }
        }
        if (inStrip) {
            AddStripToMesh(stripStart, screenEnd, startX, spriteWidth, yTop, yBottom,
                           offsetX, offsetY, cellU, cellV);
        }

        AEGfxVertexList* dynamicMesh = AEGfxMeshEnd();
        if (dynamicMesh) {
            // Camera-based exponential fog
            f32 fog = 1.0f;
            if (Camera::main && Camera::main->fogEnabled && !ignoreFog) {
                fog = expf(-proj.perpDistance / Camera::main->fogDistance);
                if (fog < Camera::main->fogMinVisibility) fog = Camera::main->fogMinVisibility;
                if (fog > Camera::main->fogMaxVisibility) fog = Camera::main->fogMaxVisibility;
            }

            AEGfxSetColorToMultiply(color.r * fog, color.g * fog, color.b * fog, color.a);

            AEMtx33 identity;
            AEMtx33Identity(&identity);
            AEGfxSetTransform(identity.m);
            AEGfxMeshDraw(dynamicMesh, AE_GFX_MDM_TRIANGLES);
            AEGfxMeshFree(dynamicMesh);
        }
    }
    else {
        // 2D mode: use AEGfxTextureSet offset to select the correct frame
        AEGfxTextureSet(tex, offsetX, offsetY);

        // ---- Standard 2D rendering ----
        f32 scaleX = transform->localScale.x;
        f32 scaleY = transform->localScale.y;

        if (flipX) scaleX = -scaleX;
        if (flipY) scaleY = -scaleY;

        AEMtx33 scale, rot, trans, result;
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
        AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
    }
}

void SpriteSheetRenderer::AddStripToMesh(int x1, int x2, f32 spriteStart, f32 spriteWidth,
                                          f32 yTop, f32 yBottom, f32 uOffsetX, f32 uOffsetY,
                                          f32 cellU, f32 cellV) {
    int sw = Application::GetWindowWidth();

    f32 sx1 = static_cast<f32>(x1) - sw / 2.0f;
    f32 sx2 = static_cast<f32>(x2 + 1) - sw / 2.0f;

    f32 spriteStartAE = spriteStart - sw / 2.0f;
    f32 spriteEndAE = spriteStartAE + spriteWidth;

    if (sx1 < spriteStartAE) sx1 = spriteStartAE;
    if (sx2 > spriteEndAE) sx2 = spriteEndAE;

    // Calculate UVs within the cell, then add the cell offset
    f32 localU1 = (sx1 - spriteStartAE) / spriteWidth * cellU + uOffsetX;
    f32 localU2 = (sx2 - spriteStartAE) / spriteWidth * cellU + uOffsetX;

    if (flipX) {
        f32 temp1 = uOffsetX + cellU - (localU1 - uOffsetX);
        f32 temp2 = uOffsetX + cellU - (localU2 - uOffsetX);
        localU1 = temp1;
        localU2 = temp2;
    }

    f32 vTop = uOffsetY;
    f32 vBottom = uOffsetY + cellV;

    if (flipY) {
        f32 tempTop = uOffsetY + cellV - (vTop - uOffsetY);
        f32 tempBottom = uOffsetY + cellV - (vBottom - uOffsetY);
        vTop = tempTop;
        vBottom = tempBottom;
    }

    u32 vertexColor = 0xFFFFFFFF;

    // Triangle 1
    AEGfxTriAdd(
        sx1, yTop, vertexColor, localU1, vTop,
        sx2, yTop, vertexColor, localU2, vTop,
        sx1, yBottom, vertexColor, localU1, vBottom
    );
    // Triangle 2
    AEGfxTriAdd(
        sx1, yBottom, vertexColor, localU1, vBottom,
        sx2, yTop, vertexColor, localU2, vTop,
        sx2, yBottom, vertexColor, localU2, vBottom
    );
}



f32 SpriteSheetRenderer::GetSortingKey() const {
    if (!transform) return static_cast<f32>(sortingOrder);

    if (Camera::main && Camera::main->transform) {
        f32 camRotRad = Camera::main->transform->rotation * Mathf::Deg2Rad;
        f32 camForwardX = cosf(camRotRad);
        f32 camForwardY = sinf(camRotRad);

        f32 dx = transform->position.x - Camera::main->transform->position.x;
        f32 dy = transform->position.y - Camera::main->transform->position.y;

        f32 dist = dx * camForwardX + dy * camForwardY;
        return dist;
    }

    return transform->position.z * 10000.0f + static_cast<f32>(sortingOrder);
}

} // namespace Engine
