#pragma once
/**
 * @file PuzzleManager.hpp
 * @brief Static manager for the book-puzzle / quiz system
 */

#include "PuzzleObject.hpp" // This brings in your Book struct!
#include <vector>
#include <string>

namespace Game {

    // Struct to hold the dynamic grade tiers loaded from JSON
    struct GradeTier {
        std::string letter;
        int maxMistakes;
        std::string text;
    };

    /**
     * @class PuzzleManager
     * @brief Owns all book data, creates in-world puzzle objects, and tracks quiz state
     */
    class PuzzleManager
    {
    public:
        // --- Core Stats & Data ---
        static std::vector<Book> allBooks;
        static std::vector<Book> Books;
        static std::vector<GradeTier> gradeTiers;

        static int booksCollected;
        static int totalWrongAnswers;

        // --- UI Text Variables ---
        static std::string activeBookText;
        static std::string activeBookTitle;
        static float popupTimer;

        static std::string activeOpt1;
        static std::string activeOpt2;
        static std::string activeOpt3;
        static std::string activeGrade;

        // --- Quiz State ---
        static bool isQuizActive;
        static int currentCorrectAnswer;
        static Engine::GameObject* currentBookObj;
        static int currentBookId;

        // --- Core Functions ---
        static void CreatePuzzleObject(Book book);
        static void BookDataInit(const std::string& filepath);
        static void Init();
        static void FreeManager();
        static void OpenExitDoor();
    };

} // namespace Game