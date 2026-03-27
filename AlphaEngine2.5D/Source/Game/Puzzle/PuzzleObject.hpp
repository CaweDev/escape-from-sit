#pragma once
/**
 * @file PuzzleObject.hpp
 * @brief Data structures and component for the book-puzzle collectibles
 */

#include "AEEngine.h"
#include "Engine/Core/Component.hpp"
#include "Engine/Math/Vector3.hpp"
#include <string>
#include <vector>

namespace Game {

/**
 * @struct Book
 * @brief Data for a single quiz / collectible book in the puzzle system
 */
struct Book {
    int bookId;
    Engine::Vector3 bookPosition;
    bool bookCollected;
    std::string bookName;
    std::string bookLore;
    std::string opt1;
    std::string opt2;
    std::string opt3;
    int correctAnswer;
};

/**
 * @class PuzzleObject
 * @brief Component attached to an in-world book collectible
 */
class PuzzleObject : public Engine::Component
{
public:
    bool puzzleCompleted;
    Book reference;
    void Update() override;
    bool isReading;

    float baseZ = 0.0f;
    void Initialize() override;
};

} // namespace Game
