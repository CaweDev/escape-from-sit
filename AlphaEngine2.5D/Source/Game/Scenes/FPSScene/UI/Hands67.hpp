#pragma once

#include "Engine/Core/Component.hpp"
#include "Engine/UI/Core/Canvas.hpp"
#include "Engine/UI/Components/SpriteSheetImage.hpp"
#include "Engine/UI/Components/Panel.hpp"

namespace Game {

    class Hands67 : public Engine::Component {
    public:
        Hands67() = default;
        ~Hands67() override = default;

        void Awake() override;
        void Update() override;

		bool IsVisible() const { return m_isVisible; }
    private:
        UI::Canvas* m_canvas = nullptr;
        UI::Panel* m_panel = nullptr;
        UI::SpriteSheetImage* m_handsImage = nullptr;
        bool m_isVisible = false;
        
        bool m_sixPressed = false;
        bool m_sevenPressed = false;
    };

} // namespace Game
