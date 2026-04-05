/*!
 * \file AlphaEngine2.5D\Source\Game\Puzzle\PuzzleManager.cpp
 */

#include "PuzzleManager.hpp"
#include "Engine.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <fstream>
#include <json/json.h>

 // ==========================================
 // CONFIGURATIONS
 // ==========================================
namespace
{
    struct PuzzleManagerConfig : public Engine::IConfig
    {
        float bookScale = 0.35f;
        float interactDistance = 2.5f;
        int spriteSortOrder = 100;
        std::string bookSpritePath = "Assets/Sprites/TempBook1.png";
        std::string interactText = "Pick up book";
        float quizPopupHoldDuration = 999.0f;
        float exitPopupDuration = 5.0f;
        std::string exitPopupTitle = "SCHOOL GATE UNLOCKED";
        std::string exitPopupBody = "Faster go home lah";

        void FromJson(const Json::Value& j) override
        {
            bookScale = j.get("bookScale", bookScale).asFloat();
            interactDistance = j.get("interactDistance", interactDistance).asFloat();
            spriteSortOrder = j.get("spriteSortOrder", spriteSortOrder).asInt();
            bookSpritePath = j.get("bookSpritePath", bookSpritePath).asString();
            interactText = j.get("interactText", interactText).asString();
            quizPopupHoldDuration = j.get("quizPopupHoldDuration", quizPopupHoldDuration).asFloat();
            exitPopupDuration = j.get("exitPopupDuration", exitPopupDuration).asFloat();
            exitPopupTitle = j.get("exitPopupTitle", exitPopupTitle).asString();
            exitPopupBody = j.get("exitPopupBody", exitPopupBody).asString();
        }
    };

    PuzzleManagerConfig g_puzzleManagerConfig;
    bool g_puzzleManagerConfigLoaded = false;

    const PuzzleManagerConfig& GetPuzzleManagerConfig()
    {
        if (!g_puzzleManagerConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/puzzle_manager.json", g_puzzleManagerConfig);
            if (g_puzzleManagerConfig.bookScale <= 0.01f) g_puzzleManagerConfig.bookScale = 0.35f;
            if (g_puzzleManagerConfig.interactDistance < 0.1f) g_puzzleManagerConfig.interactDistance = 0.1f;
            if (g_puzzleManagerConfig.quizPopupHoldDuration < 0.0f) g_puzzleManagerConfig.quizPopupHoldDuration = 0.0f;
            if (g_puzzleManagerConfig.exitPopupDuration < 0.0f) g_puzzleManagerConfig.exitPopupDuration = 0.0f;
            g_puzzleManagerConfigLoaded = true;
        }
        return g_puzzleManagerConfig;
    }
}

namespace Game {

    using namespace Engine;

    // ==========================================
    // STATIC VARIABLE INITIALIZATION
    // ==========================================
    std::vector<Book> PuzzleManager::allBooks = {};
    std::vector<Book> PuzzleManager::Books = {};
    std::vector<GradeTier> PuzzleManager::gradeTiers = {};

    int PuzzleManager::booksCollected = 0;
    int PuzzleManager::totalWrongAnswers = 0;

    std::string PuzzleManager::activeBookText = "";
    std::string PuzzleManager::activeBookTitle = "";
    float PuzzleManager::popupTimer = 0.0f;

    bool PuzzleManager::isQuizActive = false;
    int PuzzleManager::currentCorrectAnswer = 0;
    Engine::GameObject* PuzzleManager::currentBookObj = nullptr;
    int PuzzleManager::currentBookId = 0;

    std::string PuzzleManager::activeOpt1 = "";
    std::string PuzzleManager::activeOpt2 = "";
    std::string PuzzleManager::activeOpt3 = "";
    std::string PuzzleManager::activeGrade = "";


    // ==========================================
    // SPAWN LOGIC
    // ==========================================
    void PuzzleManager::CreatePuzzleObject(Book book)
    {
        const auto& cfg = GetPuzzleManagerConfig();

        auto bookObj = SceneManager::GetActiveScene()->CreateGameObject("Book");
        bookObj->transform->position = book.bookPosition;
        bookObj->transform->rotation = 0.0f;
        bookObj->transform->localScale = Vector2(cfg.bookScale, cfg.bookScale);

        auto bookPuzzleObj = bookObj->AddComponent<PuzzleObject>();

        auto bookSR = bookObj->AddComponent<SpriteRenderer>();
        bookSR->SetSprite(AssetManager::GetSprite(cfg.bookSpritePath.c_str()));
        bookSR->billboard = true;
        bookSR->enabled = true;
        bookSR->sortingOrder = cfg.spriteSortOrder;

        auto bookInteract = bookObj->AddComponent<Interactable>();
        bookInteract->actionText = cfg.interactText;
        bookInteract->maxDistance = cfg.interactDistance;

        bookInteract->onInteract = [bookPuzzleObj, bookObj, book]() {
            const auto& cfg = GetPuzzleManagerConfig();
            if (!bookPuzzleObj->isReading && !PuzzleManager::isQuizActive) {
                bookPuzzleObj->isReading = true;
                PuzzleManager::isQuizActive = true;
                PuzzleManager::currentBookObj = bookObj;
                PuzzleManager::currentBookId = book.bookId;
                PuzzleManager::currentCorrectAnswer = book.correctAnswer;
                PuzzleManager::activeBookTitle = book.bookName;
                PuzzleManager::activeBookText = book.bookLore;
                PuzzleManager::activeOpt1 = book.opt1;
                PuzzleManager::activeOpt2 = book.opt2;
                PuzzleManager::activeOpt3 = book.opt3;
                PuzzleManager::popupTimer = cfg.quizPopupHoldDuration;
            }
            };
    }


    // ==========================================
    // JSON DATA LOADER
    // ==========================================
    void PuzzleManager::BookDataInit(const std::string& filepath)
    {
        allBooks.clear();
        gradeTiers.clear();
            
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::printf("[PuzzleManager] ERROR: Could not open JSON file at %s!\n", filepath.c_str());
            return;
        }

        Json::Value root;
        try {
            file >> root;
        }
        catch (...) {
            std::printf("[PuzzleManager] ERROR: Failed to parse JSON formatting in %s!\n", filepath.c_str());
            return;
        }

        // 1. Load the Grades
        const Json::Value& gradesNode = root["grades"];
        if (gradesNode.isArray()) {
            for (int i = 0; i < (int)gradesNode.size(); ++i) {
                const Json::Value& g = gradesNode[i];
                GradeTier tier;
                tier.letter = g.get("letter", "F").asString();
                tier.maxMistakes = g.get("maxMistakes", 999).asInt();
                tier.text = g.get("text", "Missing text").asString();
                gradeTiers.push_back(tier);
            }
        }

        // 2. Load the Books
        const Json::Value& booksNode = root["books"];
        if (booksNode.isArray()) {
            for (int i = 0; i < (int)booksNode.size(); ++i) {
                const Json::Value& b = booksNode[i];

                Book newBook;
                newBook.bookId = b.get("id", i).asInt();
                newBook.bookName = b.get("name", "Unknown Book").asString();
                newBook.bookLore = b.get("lore", "Missing text.").asString();
                newBook.opt1 = b.get("opt1", "1. ").asString();
                newBook.opt2 = b.get("opt2", "2. ").asString();
                newBook.opt3 = b.get("opt3", "3. ").asString();
                newBook.correctAnswer = b.get("correctAnswer", 1).asInt();
                newBook.bookCollected = false;

                const Json::Value& posNode = b["position"];
                newBook.bookPosition = Engine::Vector3(
                    posNode.get("x", 0.0f).asFloat(),
                    posNode.get("y", 0.0f).asFloat(),
                    posNode.get("z", 0.12f).asFloat()
                );

                allBooks.push_back(newBook);
            }
        }

        std::printf("[PuzzleManager] Successfully loaded %zu books and %zu grades from %s\n", allBooks.size(), gradeTiers.size(), filepath.c_str());
    }

    void PuzzleManager::Init()
    {
        // 1. RESET GLOBALS: Clear the "Memory" of the previous game
        Game::PuzzleManager::booksCollected = 0;
        Game::PuzzleManager::totalWrongAnswers = 0;
        Game::PuzzleManager::isQuizActive = false; // This closes the UI panel on restart
        Game::PuzzleManager::popupTimer = 0.0f;
        Game::PuzzleManager::activeGrade = "";
        Game::PuzzleManager::currentBookObj = nullptr; // CRITICAL: Fixes the crash by clearing the "ghost" pointer
        Game::PuzzleManager::activeBookTitle = "";
        Game::PuzzleManager::activeBookText = "";

        // 2. RESET DATA & SPAWN: Mark all books as uncollected and create them
        for (int i{}; i < allBooks.size(); ++i) {
            allBooks[i].bookCollected = false; // Reset the JSON data state
            CreatePuzzleObject(allBooks[i]);
        }
    }

    void PuzzleManager::FreeManager()
    {
        allBooks.clear();
        booksCollected = 0;
        activeBookText.clear();
        activeBookTitle.clear();
        popupTimer = 0.0f;

        totalWrongAnswers = 0;
        activeGrade = "";
    }

    // ==========================================
    // EXIT DOOR LOGIC & GRADING
    // ==========================================
    void PuzzleManager::OpenExitDoor()
    {
        PuzzleManager::activeBookTitle = "FINAL RESULT";

        // --- THE FIX: Clear the old quiz data! ---
        PuzzleManager::activeOpt1 = "";
        PuzzleManager::activeOpt2 = "";
        PuzzleManager::activeOpt3 = "";
        PuzzleManager::isQuizActive = false; // Lock out the 1, 2, 3 number keys
        // -----------------------------------------

        // Set a default fail state just in case the JSON is missing
        PuzzleManager::activeGrade = "F";
        PuzzleManager::activeBookText = "Missing Grade Data";

        // Loop through the grades we loaded from the JSON
        for (const auto& tier : gradeTiers) {

            // Stops at the first grade tier the player qualifies for
            if (PuzzleManager::totalWrongAnswers <= tier.maxMistakes) {
                PuzzleManager::activeGrade = tier.letter;
                PuzzleManager::activeBookText = tier.text;
                break;
            }
        }

        PuzzleManager::popupTimer = 6.0f;
    }

} // namespace Game