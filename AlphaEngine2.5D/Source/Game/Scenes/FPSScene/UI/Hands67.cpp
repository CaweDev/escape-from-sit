#include "Hands67.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/AssetManager.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/UI/Core/UIManager.hpp"
#include "Engine/Core/ConfigManager.hpp"

namespace
{
    struct Hands67Config : public Engine::IConfig
    {
        int canvasSortOrder = 50;
        float referenceWidth = 1600.0f;
        float referenceHeight = 900.0f;
        float imageWidth = 1600.0f;
        float imageHeight = 900.0f;
        std::string spriteSheetPath = "Assets/Sprites/67.png";

        int toggleKeyA = static_cast<int>(Engine::KeyCode::Alpha6);
        int toggleKeyB = static_cast<int>(Engine::KeyCode::Alpha7);

        void FromJson(const Json::Value& j) override
        {
            canvasSortOrder = j.get("canvasSortOrder", canvasSortOrder).asInt();
            referenceWidth = j.get("referenceWidth", referenceWidth).asFloat();
            referenceHeight = j.get("referenceHeight", referenceHeight).asFloat();
            imageWidth = j.get("imageWidth", imageWidth).asFloat();
            imageHeight = j.get("imageHeight", imageHeight).asFloat();
            spriteSheetPath = j.get("spriteSheetPath", spriteSheetPath).asString();
            toggleKeyA = j.get("toggleKeyA", toggleKeyA).asInt();
            toggleKeyB = j.get("toggleKeyB", toggleKeyB).asInt();
        }
    };

    Hands67Config g_hands67Config;
    bool g_hands67ConfigLoaded = false;

    const Hands67Config& GetHands67Config()
    {
        if (!g_hands67ConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/hands67.json", g_hands67Config);
            g_hands67ConfigLoaded = true;
        }
        return g_hands67Config;
    }
}

namespace Game {

    void Hands67::Awake() {
        const auto& cfg = GetHands67Config();
        if (!UI::UIManager::GetInstance()) return;

        // Create canvas
        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        m_canvas->SetSortOrder(cfg.canvasSortOrder); // Below game HUD, below pause menu
        m_canvas->SetReferenceResolution(cfg.referenceWidth, cfg.referenceHeight);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);

        // Create Panel for organization
        m_panel = m_canvas->CreateChild<UI::Panel>();
        m_panel->name = "Hands67Panel";
        m_panel->SetAnchoredPosition({ 0.5f, 0.5f });
        m_panel->SetBackgroundColor(Engine::Color(0, 0, 0, 0)); // Transparent

        // Create SpriteSheetImage
        m_handsImage = m_panel->CreateChild<UI::SpriteSheetImage>();
        m_handsImage->name = "Hands67Image";
        
        // Full screen or centered large
        m_handsImage->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
        m_handsImage->SetPivot({ 0.5f, 0.5f });
        m_handsImage->SetAnchoredPosition({ 0.0f, 0.0f });
        m_handsImage->SetSizeDelta({ cfg.imageWidth, cfg.imageHeight });
        
        // Load sprite sheet
        auto* sheet = Engine::AssetManager::GetSpriteSheet(cfg.spriteSheetPath.c_str());
        if (sheet) {
            m_handsImage->SetSpriteSheet(sheet);
            m_handsImage->Play("Show");
        }

        m_panel->visible = false;
        m_isVisible = false;
    }

    void Hands67::Update() {
        const auto& cfg = GetHands67Config();
        // Toggle logic: If holding 6 and pressing 7, or holding 7 and pressing 6, or hitting both
        bool currentSix = Engine::Input::GetKey(static_cast<Engine::KeyCode>(cfg.toggleKeyA));
        bool currentSeven = Engine::Input::GetKey(static_cast<Engine::KeyCode>(cfg.toggleKeyB));
        
        // Detect a new overlap where both are held this frame, but weren't both held last frame
        bool bothPressedNow = (currentSix && currentSeven);
        bool bothPressedBefore = (m_sixPressed && m_sevenPressed);

        if (bothPressedNow && !bothPressedBefore) {
            m_isVisible = !m_isVisible;
            if (m_panel) {
                m_panel->visible = m_isVisible;
            }
        }
        
        m_sixPressed = currentSix;
        m_sevenPressed = currentSeven;
    }

} // namespace Game
