/**
 * @file AnimatedImage.cpp
 * @brief Implementation of AnimatedImage UI component
 */

#include "Engine/UI/Components/SpriteSheetImage.hpp"
#include "Engine/UI/Core/UIManager.hpp"
#include "Engine/Core/Time.hpp"

namespace UI
{
    const std::string SpriteSheetImage::s_emptyString;

    SpriteSheetImage::SpriteSheetImage()
    {
        name = "SpriteSheetImage";
    }

    SpriteSheetImage::~SpriteSheetImage()
    {
        FreeMesh();
    }

    void SpriteSheetImage::SetSpriteSheet(Engine::SpriteSheet* sheet)
    {
        m_spriteSheet = sheet;
        m_currentClip = nullptr;
        m_currentFrame = 0;
        m_elapsedTime = 0.0f;
        m_playing = false;

        FreeMesh();
        if (m_spriteSheet && m_spriteSheet->IsReady())
        {
            CreateSpritesheetMesh();
        }
    }

    void SpriteSheetImage::Play(const std::string& animationName)
    {
        if (!m_spriteSheet) return;

        const Engine::AnimationClip* clip = m_spriteSheet->GetAnimation(animationName);
        if (!clip)
        {
            printf("[Warning] AnimatedImage: Animation '%s' not found\n", animationName.c_str());
            return;
        }

        m_currentClip = clip;
        m_currentFrame = 0;
        m_elapsedTime = 0.0f;
        m_playing = true;
    }

    void SpriteSheetImage::Stop()
    {
        m_currentFrame = 0;
        m_elapsedTime = 0.0f;
        m_playing = false;
    }

    void SpriteSheetImage::Pause()
    {
        m_playing = false;
    }

    void SpriteSheetImage::Resume()
    {
        if (m_currentClip)
        {
            m_playing = true;
        }
    }

    const std::string& SpriteSheetImage::GetCurrentAnimation() const
    {
        if (m_currentClip)
        {
            return m_currentClip->name;
        }
        return s_emptyString;
    }

    void SpriteSheetImage::SetFrame(int frame)
    {
        if (m_currentClip && frame >= 0 && frame < m_currentClip->frameCount)
        {
            m_currentFrame = frame;
            m_elapsedTime = 0.0f;
        }
    }

    void SpriteSheetImage::CreateSpritesheetMesh()
    {
        FreeMesh();
        if (!m_spriteSheet) return;

        int c = m_spriteSheet->GetCols();
        int r = m_spriteSheet->GetRows();

        f32 cellU = 1.0f / static_cast<f32>(c);
        f32 cellV = 1.0f / static_cast<f32>(r);

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

        m_mesh = AEGfxMeshEnd();
    }

    void SpriteSheetImage::FreeMesh()
    {
        if (m_mesh)
        {
            AEGfxMeshFree(m_mesh);
            m_mesh = nullptr;
        }
    }

    void SpriteSheetImage::Update(f32 dt)
    {
        if (!enabled) return;

        // Advance animation
        if (m_playing && m_currentClip && m_currentClip->frameRate > 0.0f)
        {
            m_elapsedTime += dt;
            float frameDuration = 1.0f / m_currentClip->frameRate;

            while (m_elapsedTime >= frameDuration)
            {
                m_elapsedTime -= frameDuration;
                m_currentFrame++;

                if (m_currentFrame >= m_currentClip->frameCount)
                {
                    if (m_currentClip->loop)
                    {
                        m_currentFrame = 0;
                    }
                    else
                    {
                        m_currentFrame = m_currentClip->frameCount - 1;
                        m_playing = false;
                        break;
                    }
                }
            }
        }

        // Update children
        UIElement::Update(dt);
    }

    void SpriteSheetImage::Render()
    {
        if (!visible) return;
        if (!m_spriteSheet || !m_spriteSheet->IsReady() || !m_mesh) return;
        if (!m_currentClip) return;

        // Compute UV offset for current frame
        int absFrame = m_currentClip->startFrame + m_currentFrame;
        int c = m_spriteSheet->GetCols();
        int r = m_spriteSheet->GetRows();

        f32 cellU = 1.0f / static_cast<f32>(c);
        f32 cellV = 1.0f / static_cast<f32>(r);

        f32 offsetX = static_cast<f32>(absFrame % c) * cellU;
        f32 offsetY = static_cast<f32>(absFrame / c) * cellV;

        // Get world rect from UI layout system
        Rect worldRect = GetWorldRect();

        // Calculate transform matrix
        AEMtx33 scale, translate, transform;
        AEMtx33Scale(&scale, worldRect.width, worldRect.height);

        AEMtx33Trans(&translate, worldRect.GetCenterX(), worldRect.GetCenterY());
        AEMtx33Concat(&transform, &translate, &scale);

        // Set render state
        Engine::Texture2D* tex = m_spriteSheet->GetTexture();
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(tex->GetNativeTexture(), offsetX, offsetY);

        // Set color
        const Engine::Color& col = GetColor();
        AEGfxSetColorToMultiply(col.r, col.g, col.b, col.a);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(col.a);

        // Draw with owned mesh (not UIManager::GetQuadMesh())
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(m_mesh, AE_GFX_MDM_TRIANGLES);

        // Render children
        UIElement::Render();
    }

} // namespace UI
