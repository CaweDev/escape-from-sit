/**
 * @file Image.cpp
 * @brief Image component implementation
 */

#include "Engine/UI/Components/Image.hpp"
#include "Engine/UI/Core/UIManager.hpp"

namespace UI
{
    Image::Image()
        : m_color(Engine::Color::white())
    {
        name = "Image";
    }

    void Image::Render()
    {
        if (!visible) return;

        Rect worldRect = GetWorldRect();

        // Calculate transform matrix
        AEMtx33 scale, translate, transform;
        AEMtx33Scale(&scale, worldRect.width * m_fillAmount, worldRect.height);
        
        // Adjust position for fill amount (anchored to left)
        f32 fillOffset = worldRect.width * (1.0f - m_fillAmount) * 0.5f;
        AEMtx33Trans(&translate, worldRect.GetCenterX() - fillOffset, worldRect.GetCenterY());
        AEMtx33Concat(&transform, &translate, &scale);

        // Set render state
        if (m_texture && m_texture->IsLoaded())
        {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxTextureSet(m_texture->GetNativeTexture(), 0, 0);
        }
        else
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        }

        // Set color
        AEGfxSetColorToMultiply(m_color.r, m_color.g, m_color.b, m_color.a);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(m_color.a);

        // Set transform and draw
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(UIManager::GetQuadMesh(), AE_GFX_MDM_TRIANGLES);

        // Render children
        UIElement::Render();
    }

} // namespace UI
