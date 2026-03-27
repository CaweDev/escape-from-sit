/**
 * @file Crosshair.cpp
 * @brief Implementation of Crosshair UI element
 */
#include "Engine/UI/Specialized/Crosshair.hpp"
#include "Engine/Core/ConfigManager.hpp"

namespace
{
    struct CrosshairConfig : public Engine::IConfig
    {
        float anchorX = 0.5f;
        float anchorY = 0.5f;
        float pivotX = 0.5f;
        float pivotY = 0.5f;
        float posX = 0.0f;
        float posY = 0.0f;
        float width = 32.0f;
        float height = 32.0f;
        int renderOrder = 100000;

        void FromJson(const Json::Value& j) override
        {
            anchorX = j.get("anchorX", anchorX).asFloat();
            anchorY = j.get("anchorY", anchorY).asFloat();
            pivotX = j.get("pivotX", pivotX).asFloat();
            pivotY = j.get("pivotY", pivotY).asFloat();
            posX = j.get("posX", posX).asFloat();
            posY = j.get("posY", posY).asFloat();
            width = j.get("width", width).asFloat();
            height = j.get("height", height).asFloat();
            renderOrder = j.get("renderOrder", renderOrder).asInt();
        }
    };

    CrosshairConfig g_crosshairConfig;
    bool g_crosshairConfigLoaded = false;

    const CrosshairConfig& GetCrosshairConfig()
    {
        if (!g_crosshairConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Engine/crosshair.json", g_crosshairConfig);
            g_crosshairConfigLoaded = true;
        }
        return g_crosshairConfig;
    }
}

namespace UI {

Crosshair::Crosshair() {
    const CrosshairConfig& cfg = GetCrosshairConfig();
    // Center the crosshair on screen by default
    SetAnchors({ cfg.anchorX, cfg.anchorY }, { cfg.anchorX, cfg.anchorY });
    SetPivot({ cfg.pivotX, cfg.pivotY });
    SetAnchoredPosition({ cfg.posX, cfg.posY });
    SetSizeDelta({ cfg.width, cfg.height }); // Default size
    
    // Crosshair shouldn't block raycasts
    raycastTarget = false;

    m_renderOrder = cfg.renderOrder;
    
    name = "Crosshair";
}

void Crosshair::SetDefaultTexture(Engine::Texture2D* texture) {
    m_defaultTexture = texture;
    UpdateTexture();
}

void Crosshair::SetHighlightTexture(Engine::Texture2D* texture) {
    m_highlightTexture = texture;
    UpdateTexture();
}

void Crosshair::SetHighlighted(bool highlighted) {
    if (m_highlighted != highlighted) {
        m_highlighted = highlighted;
        UpdateTexture();
    }
}

void Crosshair::UpdateTexture() {
    if (m_highlighted && m_highlightTexture) {
        SetTexture(m_highlightTexture);
    } else {
        SetTexture(m_defaultTexture);
    }
}

} // namespace UI
