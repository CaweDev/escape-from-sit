/**
 * @file ConfirmationDialog.cpp
 * @brief ConfirmationDialog implementation
 */

#include "Engine/UI/GameObject/ConfirmationDialog.hpp"
#include "Engine/UI/Core/UIManager.hpp"
#include "Engine/UI/Core/Canvas.hpp"
#include "Engine/UI/Components/Panel.hpp"
#include "Engine/UI/Components/Text.hpp"
#include "Engine/UI/Components/Button.hpp"
#include "Engine/UI/Components/Image.hpp"
#include "Engine/Core/AssetManager.hpp"
#include "Engine/Core/ConfigManager.hpp"

namespace
{
    struct ConfirmationDialogConfig : public Engine::IConfig
    {
        int canvasSortOrder = 100000;
        float referenceWidth = 1280.0f;
        float referenceHeight = 720.0f;

        float panelWidth = 420.0f;
        float panelHeight = 220.0f;

        float messageAnchorMinY = 0.4f;
        float messageOffsetY = -10.0f;
        float messageWidthInset = -40.0f;
        float messageFontSize = 1.2f;

        float buttonWidth = 140.0f;
        float buttonHeight = 45.0f;
        float buttonOffsetY = 50.0f;
        float buttonLabelFontSize = 1.1f;

        float panelPixelsPerUnitMultiplier = 2.0f;
        float buttonPixelsPerUnitMultiplier = 1.0f;

        std::string panelSpritePath = "Assets/UI/UI Kit/Panel/panel-001.png";
        std::string buttonSpritePath = "Assets/UI/UI Kit/Panel/panel-027.png";

        Engine::Color overlayColor = Engine::Color(0.0f, 0.0f, 0.0f, 0.72f);
        Engine::Color panelColor = Engine::Color(0.2f, 0.2f, 0.2f, 0.9f);
        Engine::Color confirmButtonColor = Engine::Color(0.2f, 0.6f, 0.2f, 1.0f);
        Engine::Color cancelButtonColor = Engine::Color(0.6f, 0.2f, 0.2f, 1.0f);

        static Engine::Color ParseColor(const Json::Value& j, const char* key, const Engine::Color& fallback)
        {
            if (!j.isMember(key) || !j[key].isObject()) return fallback;
            const Json::Value& c = j[key];
            return Engine::Color(
                c.get("r", fallback.r).asFloat(),
                c.get("g", fallback.g).asFloat(),
                c.get("b", fallback.b).asFloat(),
                c.get("a", fallback.a).asFloat());
        }

        void FromJson(const Json::Value& j) override
        {
            canvasSortOrder = j.get("canvasSortOrder", canvasSortOrder).asInt();
            referenceWidth = j.get("referenceWidth", referenceWidth).asFloat();
            referenceHeight = j.get("referenceHeight", referenceHeight).asFloat();

            panelWidth = j.get("panelWidth", panelWidth).asFloat();
            panelHeight = j.get("panelHeight", panelHeight).asFloat();

            messageAnchorMinY = j.get("messageAnchorMinY", messageAnchorMinY).asFloat();
            messageOffsetY = j.get("messageOffsetY", messageOffsetY).asFloat();
            messageWidthInset = j.get("messageWidthInset", messageWidthInset).asFloat();
            messageFontSize = j.get("messageFontSize", messageFontSize).asFloat();

            buttonWidth = j.get("buttonWidth", buttonWidth).asFloat();
            buttonHeight = j.get("buttonHeight", buttonHeight).asFloat();
            buttonOffsetY = j.get("buttonOffsetY", buttonOffsetY).asFloat();
            buttonLabelFontSize = j.get("buttonLabelFontSize", buttonLabelFontSize).asFloat();

            panelPixelsPerUnitMultiplier = j.get("panelPixelsPerUnitMultiplier", panelPixelsPerUnitMultiplier).asFloat();
            buttonPixelsPerUnitMultiplier = j.get("buttonPixelsPerUnitMultiplier", buttonPixelsPerUnitMultiplier).asFloat();

            panelSpritePath = j.get("panelSpritePath", panelSpritePath).asString();
            buttonSpritePath = j.get("buttonSpritePath", buttonSpritePath).asString();

            overlayColor = ParseColor(j, "overlayColor", overlayColor);
            panelColor = ParseColor(j, "panelColor", panelColor);
            confirmButtonColor = ParseColor(j, "confirmButtonColor", confirmButtonColor);
            cancelButtonColor = ParseColor(j, "cancelButtonColor", cancelButtonColor);
        }
    };

    ConfirmationDialogConfig g_confirmationDialogConfig;
    bool g_confirmationDialogConfigLoaded = false;

    const ConfirmationDialogConfig& GetConfirmationDialogConfig()
    {
        if (!g_confirmationDialogConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Engine/confirmation_dialog.json", g_confirmationDialogConfig);
            g_confirmationDialogConfigLoaded = true;
        }
        return g_confirmationDialogConfig;
    }
}

namespace UI
{
    void ConfirmationDialog::Awake()
    {
        Build();
    }

    void ConfirmationDialog::Build()
    {
        if (m_built) return;

        UIManager* mgr = UIManager::GetInstance();
        if (!mgr) return;
        const auto& cfg = GetConfirmationDialogConfig();

        // --- Create a dedicated modal canvas ---
        m_canvas = mgr->CreateCanvas();
        m_canvas->SetSortOrder(cfg.canvasSortOrder);
        m_canvas->SetReferenceResolution(cfg.referenceWidth, cfg.referenceHeight);
        m_canvas->SetScaleMode(CanvasScaleMode::ScaleWithScreenSize);

        // --- Full-screen dark backdrop (blocks clicks behind dialog) ---
        m_overlay = m_canvas->CreateChild<Panel>();
        m_overlay->name = "ConfirmDialog_DarkPanel";
        m_overlay->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_overlay->SetAnchoredPosition({ 0.0f, 0.0f });
        m_overlay->SetSizeDelta({ 0.0f, 0.0f });
        m_overlay->SetBackgroundColor(cfg.overlayColor);
        m_overlay->raycastTarget = true;
        m_overlay->SetRenderOrder(10);

        // --- Centered dialog panel ---
        m_dialogPanel = m_overlay->CreateChild<Panel>();
        m_dialogPanel->name = "ConfirmDialog_Panel";
        m_dialogPanel->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
        m_dialogPanel->SetPivot({ 0.5f, 0.5f });
        m_dialogPanel->SetAnchoredPosition({ 0.0f, 0.0f });
        m_dialogPanel->SetSizeDelta({ cfg.panelWidth, cfg.panelHeight });
        m_dialogPanel->SetRenderOrder(20);

        // Apply panel-001.png as background with 9-slice
        m_dialogPanel->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.panelSpritePath.c_str()));
        m_dialogPanel->SetBackgroundColor(cfg.panelColor);
        if (auto* bg = m_dialogPanel->GetBackground())
        {
            bg->SetImageType(ImageType::Sliced);
            bg->SetPixelsPerUnitMultiplier(cfg.panelPixelsPerUnitMultiplier);
        }

        m_dialogPanel->raycastTarget = true;

        // --- Message text (upper portion of dialog) ---
        m_messageText = m_dialogPanel->CreateChild<Text>();
        m_messageText->name = "ConfirmDialog_Message";
        m_messageText->SetAnchors({ 0.0f, cfg.messageAnchorMinY }, { 1.0f, 1.0f });
        m_messageText->SetAnchoredPosition({ 0.0f, cfg.messageOffsetY });
        m_messageText->SetSizeDelta({ cfg.messageWidthInset, 0.0f });
        m_messageText->SetAnchor(TextAnchor::MiddleCenter);
        m_messageText->SetColor(Engine::Color::white());
        m_messageText->SetFontSize(cfg.messageFontSize);
        m_messageText->raycastTarget = false;
        m_messageText->SetText("Are you sure?");

        // --- Confirm button (left side of lower area) ---
        m_confirmBtn = m_dialogPanel->CreateChild<Button>();
        m_confirmBtn->name = "ConfirmDialog_ConfirmBtn";
        m_confirmBtn->SetAnchors({ 0.25f, 0.0f }, { 0.25f, 0.0f });
        m_confirmBtn->SetPivot({ 0.5f, 0.5f });
        m_confirmBtn->SetAnchoredPosition({ 0.0f, cfg.buttonOffsetY });
        m_confirmBtn->SetSizeDelta({ cfg.buttonWidth, cfg.buttonHeight });
        
        // Apply panel-022.png to buttons (9-slice enabled by default in SetBackgroundSprite)
        m_confirmBtn->SetBackgroundColor(cfg.confirmButtonColor);
        m_confirmBtn->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.buttonSpritePath.c_str()), true);
        if (auto* bg = m_confirmBtn->GetBackground())
        {
            bg->SetPixelsPerUnitMultiplier(cfg.buttonPixelsPerUnitMultiplier);
        }
        
        m_confirmBtn->SetText("Confirm");
        if (auto* label = m_confirmBtn->GetLabel())
        {
            label->SetFontSize(cfg.buttonLabelFontSize);
        }
        m_confirmBtn->onClick = [this]() { OnConfirmClicked(); };

        // --- Cancel button (right side of lower area) ---
        m_cancelBtn = m_dialogPanel->CreateChild<Button>();
        m_cancelBtn->name = "ConfirmDialog_CancelBtn";
        m_cancelBtn->SetAnchors({ 0.75f, 0.0f }, { 0.75f, 0.0f });
        m_cancelBtn->SetPivot({ 0.5f, 0.5f });
        m_cancelBtn->SetAnchoredPosition({ 0.0f, cfg.buttonOffsetY });
        m_cancelBtn->SetSizeDelta({ cfg.buttonWidth, cfg.buttonHeight });
        
        // Apply panel-022.png to buttons
        m_cancelBtn->SetBackgroundColor(cfg.cancelButtonColor);
        m_cancelBtn->SetBackgroundSprite(Engine::AssetManager::GetTexture(cfg.buttonSpritePath.c_str()), true);
        if (auto* bg = m_cancelBtn->GetBackground())
        {
            bg->SetPixelsPerUnitMultiplier(cfg.buttonPixelsPerUnitMultiplier);
        }
        
        m_cancelBtn->SetText("Cancel");
        if (auto* label = m_cancelBtn->GetLabel())
        {
            label->SetFontSize(cfg.buttonLabelFontSize);
        }
        m_cancelBtn->onClick = [this]() { OnCancelClicked(); };

        // Start hidden
        m_overlay->visible = false;
        m_overlay->enabled = false;

        m_built = true;
    }

    void ConfirmationDialog::Show(const std::string& message,
                                   const std::string& confirmText,
                                   const std::string& cancelText,
                                   std::function<void()> onConfirm,
                                   std::function<void()> onCancel)
    {
        if (!m_built) Build();
        if (!m_overlay) return;

        // Set content
        m_messageText->SetText(message);
        m_confirmBtn->SetText(confirmText);
        m_cancelBtn->SetText(cancelText);

        // Store callbacks
        m_onConfirm = onConfirm;
        m_onCancel = onCancel;

        // Show the dialog
        m_overlay->visible = true;
        m_overlay->enabled = true;
    }

    void ConfirmationDialog::Hide()
    {
        if (!m_overlay) return;

        m_overlay->visible = false;
        m_overlay->enabled = false;

        // Clear callbacks to avoid dangling references
        m_onConfirm = nullptr;
        m_onCancel = nullptr;
    }

    bool ConfirmationDialog::IsVisible() const
    {
        return m_overlay && m_overlay->visible;
    }

    void ConfirmationDialog::OnConfirmClicked()
    {
        auto callback = m_onConfirm;
        Hide();
        if (callback)
        {
            callback();
        }
    }

    void ConfirmationDialog::OnCancelClicked()
    {
        auto callback = m_onCancel;
        Hide();
        if (callback)
        {
            callback();
        }
    }

    void ConfirmationDialog::SetBackground(Engine::Texture2D* texture)
    {
        if (!m_built) Build();
        if (!m_dialogPanel || !texture) return;

        m_dialogPanel->SetBackgroundSprite(texture);

        // Enable 9-slice on the background image
        Image* bg = m_dialogPanel->GetBackground();
        if (bg)
        {
            bg->SetImageType(ImageType::Sliced);
        }
    }

} // namespace UI
