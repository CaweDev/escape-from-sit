/**
 * @file DialogService.cpp
 * @brief DialogService implementation
 */

#include "Engine/UI/Helpers/DialogService.hpp"
#include "Engine/UI/GameObject/ConfirmationDialog.hpp"

namespace UI
{
    ConfirmationDialog* DialogService::s_confirmDialog = nullptr;

    void DialogService::Initialize()
    {
        if (!s_confirmDialog)
        {
            s_confirmDialog = new ConfirmationDialog();
            s_confirmDialog->Awake();
        }
    }

    void DialogService::Shutdown()
    {
        delete s_confirmDialog;
        s_confirmDialog = nullptr;
    }

    void DialogService::ShowConfirmation(
        const std::string& message,
        const std::string& confirmText,
        const std::string& cancelText,
        std::function<void()> onConfirm,
        std::function<void()> onCancel)
    {
        if (!s_confirmDialog) Initialize();
        if (s_confirmDialog)
        {
            s_confirmDialog->Show(message, confirmText, cancelText, onConfirm, onCancel);
        }
    }

    void DialogService::HideConfirmation()
    {
        if (s_confirmDialog)
        {
            s_confirmDialog->Hide();
        }
    }

    bool DialogService::IsConfirmationVisible()
    {
        return s_confirmDialog && s_confirmDialog->IsVisible();
    }

    void DialogService::SetConfirmationBackground(Engine::Texture2D* texture)
    {
        if (!s_confirmDialog) Initialize();
        if (s_confirmDialog)
        {
            s_confirmDialog->SetBackground(texture);
        }
    }

} // namespace UI
