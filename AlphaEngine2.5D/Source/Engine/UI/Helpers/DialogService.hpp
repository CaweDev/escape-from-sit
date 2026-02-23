#pragma once
/**
 * @file DialogService.hpp
 * @brief Lightweight static service for modal dialog management
 *
 * Separated from UIManager to keep concerns narrow. Owns the
 * ConfirmationDialog singleton and delegates calls to it.
 */

#include <string>
#include <functional>

namespace Engine { class Texture2D; }

namespace UI
{
    class ConfirmationDialog;

    /**
     * @class DialogService
     * @brief Static service for modal UI dialogs
     *
     * Provides a global entry point for showing/hiding confirmation dialogs
     * without coupling to UIManager.
     */
    class DialogService
    {
    public:
        /**
         * @brief Initialize the dialog service (called once at engine startup)
         */
        static void Initialize();

        /**
         * @brief Shut down the dialog service and free resources
         */
        static void Shutdown();

        // ===== Confirmation Dialog =====

        /**
         * @brief Show a modal confirmation dialog
         * @param message       Message text to display
         * @param confirmText   Confirm button label
         * @param cancelText    Cancel button label
         * @param onConfirm     Callback on confirm
         * @param onCancel      Callback on cancel (optional)
         */
        static void ShowConfirmation(
            const std::string& message,
            const std::string& confirmText,
            const std::string& cancelText,
            std::function<void()> onConfirm,
            std::function<void()> onCancel = nullptr);

        /**
         * @brief Hide the confirmation dialog
         */
        static void HideConfirmation();

        /**
         * @brief Check if the confirmation dialog is visible
         */
        static bool IsConfirmationVisible();

        /**
         * @brief Set a 9-slice background texture for the confirmation dialog
         * @param texture Texture with NineSliceBorder already configured
         */
        static void SetConfirmationBackground(Engine::Texture2D* texture);

    private:
        static ConfirmationDialog* s_confirmDialog;
    };

} // namespace UI
