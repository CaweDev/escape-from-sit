/**
 * @file UITestScene.hpp
 * @brief Scene for testing the new UI framework
 */
#pragma once

#include "Engine/Scene/Scene.hpp"
#include "Engine/UI/UI.hpp"

namespace Game {

using namespace Engine;
using namespace UI;

/**
 * @class UITestScene
 * @brief Demonstrates and verifies all UI components
 */
class UITestScene : public Scene {
public:
    UITestScene();
    virtual ~UITestScene();

    void OnLoad() override;
    void OnUnload() override;
    void OnUpdate() override;

private:
    Canvas* mainCanvas = nullptr;
    Panel* panel;
};

} // namespace Game
