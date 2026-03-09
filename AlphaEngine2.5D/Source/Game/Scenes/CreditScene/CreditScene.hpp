#pragma once
/**
 * @file CreditScene.hpp
 * @brief Scene showing panning credits
 */

#include "Engine.hpp"

namespace Game {

using namespace Engine;
using namespace UI;

class CreditScene : public Scene {
public:
    CreditScene();

    void OnLoad() override;
    void OnUnload() override;
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

    void SkipToEnd();
    bool IsSkipPressed() const;
};

} // namespace Game
