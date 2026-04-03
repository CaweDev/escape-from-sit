/**
 * @file InteractionPrompt.cpp
 * @brief Implementation of InteractionPrompt UI element
 */
#include "Engine/UI/Specialized/InteractionPrompt.hpp"
#include "Engine/Core/ConfigManager.hpp"

namespace
{
    struct InteractionPromptConfig : public Engine::IConfig
    {
        float anchorX = 0.5f;
        float anchorY = 0.5f;
        float pivotX = 0.5f;
        float pivotY = 1.0f;
        float posX = 0.0f;
        float posY = -40.0f;
        float width = 200.0f;
        float height = 30.0f;
        float fontSize = 1.0f;
        Engine::Color textColor = Engine::Color::white();

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
            fontSize = j.get("fontSize", fontSize).asFloat();
            if (j.isMember("textColor") && j["textColor"].isObject())
            {
                const Json::Value& c = j["textColor"];
                textColor = Engine::Color(
                    c.get("r", textColor.r).asFloat(),
                    c.get("g", textColor.g).asFloat(),
                    c.get("b", textColor.b).asFloat(),
                    c.get("a", textColor.a).asFloat());
            }
        }
    };

    InteractionPromptConfig g_promptConfig;
    bool g_promptConfigLoaded = false;

    const InteractionPromptConfig& GetInteractionPromptConfig()
    {
        if (!g_promptConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Engine/interaction_prompt.json", g_promptConfig);
            g_promptConfigLoaded = true;
        }
        return g_promptConfig;
    }
}

namespace UI {

InteractionPrompt::InteractionPrompt() {
    const auto& cfg = GetInteractionPromptConfig();
    // Position below screen center (where crosshair is)
    SetAnchors({ cfg.anchorX, cfg.anchorY }, { cfg.anchorX, cfg.anchorY });
    SetPivot({ cfg.pivotX, cfg.pivotY });
    SetAnchoredPosition({ cfg.posX, cfg.posY });
    SetSizeDelta({ cfg.width, cfg.height });
    
    // Not a raycast target
    raycastTarget = false;
    
    // Create text child using hierarchy
    m_text = CreateChild<Text>();
    m_text->SetAnchor(TextAnchor::MiddleCenter);
    m_text->SetColor(cfg.textColor);
    m_text->SetFontSize(cfg.fontSize);
    
    // Fill parent
    m_text->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
    m_text->SetSizeDelta({ 0.0f, 0.0f });
    
    // Start hidden
    visible = false;
    
    name = "InteractionPrompt";
}



void InteractionPrompt::Show(const std::string& actionText) {
    m_currentActionText = actionText;
    UpdateText();
    visible = true;
}


void InteractionPrompt::Hide() {
    visible = false;
}



void InteractionPrompt::SetKeyText(const std::string& keyText) {
    m_keyText = keyText;
    UpdateText();
}



void InteractionPrompt::SetTextColor(const Engine::Color& color) {
    if (m_text) {
        m_text->SetColor(color);
    }
}



void InteractionPrompt::SetFontSize(f32 size) {
    if (m_text) {
        m_text->SetFontSize(size);
    }
}


void InteractionPrompt::UpdateText() {
    if (m_text) {
        m_text->SetText(m_keyText + " " + m_currentActionText);
    }
}

} // namespace UI
