#pragma once
/**
 * @file ConfirmationDialog.hpp
 * @brief Modal confirmation dialog that inherits from Component
 *
 * Creates and owns its own Canvas + UI hierarchy (overlay, dialog panel,
 * text, buttons). Managed by DialogService.
 */

#include "Engine/Core/Component.hpp"
#include "Engine/Graphics/Color.hpp"
#include <string>
#include <functional>

namespace Engine { class Texture2D; }

// Forward declarations
namespace UI
{
    class Canvas;
    class Panel;
    class Text;
    class Button;
}

namespace UI
{
    /**
     * @class ConfirmationDialog
     * @brief Modal confirmation dialog with message text and confirm/cancel buttons
     *
     * Inherits from Engine::Component so it participates in the GameObject lifecycle.
     *
     * Hierarchy:
     *   Canvas (sortOrder = 9998)
     *   └── Panel (overlay, full-screen semi-transparent black)
     *       └── Panel (dialog box, centered)
     *           ├── Text (message)
     *           ├── Button (confirm)
     *           └── Button (cancel)
     */
    class ConfirmationDialog : public Engine::Component
    {
    private:
        Canvas* m_canvas      = nullptr;
        Panel*  m_overlay     = nullptr;  // Full-screen dim overlay
        Panel*  m_dialogPanel = nullptr;  // Centered dialog box
        Text*   m_messageText = nullptr;
        Button* m_confirmBtn  = nullptr;
        Button* m_cancelBtn   = nullptr;

        std::function<void()> m_onConfirm;
        std::function<void()> m_onCancel;

        bool m_built = false;

    public:
        ConfirmationDialog() = default;
        virtual ~ConfirmationDialog() = default;

        /**
         * @brief Component lifecycle — builds the UI hierarchy
         */
        void Awake() override;

        /**
         * @brief Show the dialog with the given parameters
         * @param message       Message text to display
         * @param confirmText   Label for the confirm button
         * @param cancelText    Label for the cancel button
         * @param onConfirm     Callback invoked when confirm is clicked
         * @param onCancel      Callback invoked when cancel is clicked (optional)
         */
        void Show(const std::string& message,
                  const std::string& confirmText,
                  const std::string& cancelText,
                  std::function<void()> onConfirm,
                  std::function<void()> onCancel = nullptr);

        /**
         * @brief Hide the dialog and clear callbacks
         */
        void Hide();

        /**
         * @brief Check if the dialog is currently visible
         */
        bool IsVisible() const;

        /**
         * @brief Set a 9-slice background texture for the dialog panel
         * @param texture Texture with NineSliceBorder already set
         */
        void SetBackground(Engine::Texture2D* texture);

    private:
        /**
         * @brief Builds the Canvas + UI element hierarchy
         */
        void Build();

        /**
         * @brief Internal handler for confirm button click
         */
        void OnConfirmClicked();

        /**
         * @brief Internal handler for cancel button click
         */
        void OnCancelClicked();
    };

} // namespace UI
