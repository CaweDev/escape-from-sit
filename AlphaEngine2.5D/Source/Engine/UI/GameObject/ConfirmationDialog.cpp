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

        // --- Create a dedicated canvas just below the fade overlay ---
        m_canvas = mgr->CreateCanvas();
        m_canvas->SetSortOrder(100000);
        m_canvas->SetReferenceResolution(1280.f, 720.f);
        m_canvas->SetScaleMode(CanvasScaleMode::ScaleWithScreenSize);

        // --- Full-screen semi-transparent overlay (blocks clicks behind dialog) ---
        m_overlay = m_canvas->CreateChild<Panel>();
        m_overlay->name = "ConfirmDialog_Overlay";
        m_overlay->SetAnchors({ 0.0f, 0.0f }, { 1.0f, 1.0f });
        m_overlay->SetAnchoredPosition({ 0.0f, 0.0f });
        m_overlay->SetSizeDelta({ 0.0f, 0.0f });
        m_overlay->SetBackgroundColor(Engine::Color(0.0f, 0.0f, 0.0f, 0.6f));
        m_overlay->raycastTarget = true;  // Block clicks behind the dialog
        m_overlay->SetRenderOrder(1);

        // --- Centered dialog panel ---
        m_dialogPanel = m_overlay->CreateChild<Panel>();
        m_dialogPanel->name = "ConfirmDialog_Panel";
        m_dialogPanel->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
        m_dialogPanel->SetPivot({ 0.5f, 0.5f });
        m_dialogPanel->SetAnchoredPosition({ 0.0f, 0.0f });
        m_dialogPanel->SetSizeDelta({ 420.0f, 220.0f });
        m_dialogPanel->SetBackgroundColor(Engine::Color(0.15f, 0.15f, 0.2f, 0.95f));
        m_dialogPanel->raycastTarget = true;

        // --- Message text (upper portion of dialog) ---
        m_messageText = m_dialogPanel->CreateChild<Text>();
        m_messageText->name = "ConfirmDialog_Message";
        m_messageText->SetAnchors({ 0.0f, 0.4f }, { 1.0f, 1.0f });
        m_messageText->SetAnchoredPosition({ 0.0f, -10.0f });
        m_messageText->SetSizeDelta({ -40.0f, 0.0f });
        m_messageText->SetAnchor(TextAnchor::MiddleCenter);
        m_messageText->SetColor(Engine::Color::white());
        m_messageText->SetFontSize(1.2f);
        m_messageText->raycastTarget = false;
        m_messageText->SetText("Are you sure?");

        // --- Confirm button (left side of lower area) ---
        m_confirmBtn = m_dialogPanel->CreateChild<Button>();
        m_confirmBtn->name = "ConfirmDialog_ConfirmBtn";
        m_confirmBtn->SetAnchors({ 0.25f, 0.0f }, { 0.25f, 0.0f });
        m_confirmBtn->SetPivot({ 0.5f, 0.5f });
        m_confirmBtn->SetAnchoredPosition({ 0.0f, 50.0f });
        m_confirmBtn->SetSizeDelta({ 140.0f, 45.0f });
        m_confirmBtn->SetBackgroundColor(Engine::Color(0.2f, 0.6f, 0.3f, 1.0f));
        m_confirmBtn->SetText("Confirm");
        m_confirmBtn->onClick = [this]() { OnConfirmClicked(); };

        // --- Cancel button (right side of lower area) ---
        m_cancelBtn = m_dialogPanel->CreateChild<Button>();
        m_cancelBtn->name = "ConfirmDialog_CancelBtn";
        m_cancelBtn->SetAnchors({ 0.75f, 0.0f }, { 0.75f, 0.0f });
        m_cancelBtn->SetPivot({ 0.5f, 0.5f });
        m_cancelBtn->SetAnchoredPosition({ 0.0f, 50.0f });
        m_cancelBtn->SetSizeDelta({ 140.0f, 45.0f });
        m_cancelBtn->SetBackgroundColor(Engine::Color(0.6f, 0.2f, 0.2f, 1.0f));
        m_cancelBtn->SetText("Cancel");
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
