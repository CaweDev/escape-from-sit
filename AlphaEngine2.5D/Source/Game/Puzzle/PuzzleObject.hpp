// Inside PuzzleObject.hpp
#pragma once
#include "Engine/Core/Component.hpp"
#include "Engine/Math/Vector3.hpp"
#include <string>

namespace Game {

    struct Book {
        int bookId = 0;
        Engine::Vector3 bookPosition = { 0.0f, 0.0f, 0.0f };
        bool bookCollected = false; // Must be lowercase 'b'!
        std::string bookName = "";
        std::string bookLore = "";
        std::string opt1 = "";
        std::string opt2 = "";
        std::string opt3 = "";
        int correctAnswer = 1;
    };

    class PuzzleObject : public Engine::Component {
    public:
        bool isReading = false;
        bool puzzleCompleted = false;
        float baseZ = 0.0f; // The anchor for our floating animation

        void Initialize() override;
        void Update() override;
    };

} // namespace Game