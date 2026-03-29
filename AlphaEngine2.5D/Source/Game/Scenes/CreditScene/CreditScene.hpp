/*! 
 * \file CreditScene.hpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief Scene showing panning credits
 */

#pragma once

#include "Engine.hpp"

namespace Game {

using namespace Engine;
using namespace UI;

/*!
 * \class CreditScene
 * \brief Scene that displays scrolling/panning credits with skip support
 */
class CreditScene : public Scene {
public:
    /*! \brief Construct credit scene. */
    CreditScene();

    /*! \brief Initialize UI and start credit scroll. */
    void OnLoad() override;

    /*! \brief Cleanup created UI on unload. */
    void OnUnload() override;

    /*! \brief Per-frame update to advance scrolling and handle skip. */
    void OnUpdate() override;

private:
    Canvas* m_canvas = nullptr;
    Panel*  m_background = nullptr;
    Image*  m_creditsImage = nullptr;

    float m_scrollSpeed = 100.0f; // Pixels per second in canvas-space
    float m_currentY = 0.0f;
    float m_targetY = 0.0f;

    bool m_transitioning = false;

    static float CANVAS_REF_WIDTH;
    static float CANVAS_REF_HEIGHT;
    static const char* CREDITS_TEXTURE_PATH;

    /*! \brief Immediately skip to the end and transition. */
    void SkipToEnd();

    /*! \brief Check if skip input was pressed. \return true if skip requested */
    bool IsSkipPressed() const;
};

} // namespace Game
