#include "PuzzleManager.hpp"
#include "Engine.hpp"
#include "Engine/Audio/ProximityAudio.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <array>

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

std::vector<Book> PuzzleManager::allBooks = {};
int PuzzleManager::booksCollected = 0;

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

void PuzzleManager::BookDataInit()
{
    std::array<std::string, 6> bookNames = { "POP QUIZ 1", "POP QUIZ 2", "POP QUIZ 3", "POP QUIZ 4", "POP QUIZ 5", "POP QUIZ 6" };

    std::array<std::string, 6> bookLore = {
        "What does 'int*' represent?",
        "What is a memory leak?",
        "Which of these is a Loop?",
        "|Cinema|",
        "Question 5?",
        "Question 6?"
    };

    std::array<std::string, 6> opt1 = { "1. A memory address", "1. Spilled RAM", "1. if()", "1. Absolute Cinema", "1. A", "1. A" };
    std::array<std::string, 6> opt2 = { "2. A decimal number", "2. Unfreed memory", "2. switch()", "2. ???", "2. B", "2. B" };
    std::array<std::string, 6> opt3 = { "3. A boolean", "3. A fast CPU", "3. while()", "3. Cinema?", "3. C", "3. C" };

    int correctAnswers[6] = { 1, 2, 3, 1, 1, 3 };

    Engine::Vector3 bookCoords[6] = { {8.0f, 17.0f, 0.12f} ,{9.0f, 41.0f, 0.12f},{90.f, 40.f, 0.12f},{121.0f, 19.f, 0.12f},{83.0f, 12.0f, 0.12f},{73.0f, 19.0f, 0.12f} };

    for (int i{}; i < 6; ++i) {
        allBooks.push_back({
        i,
        bookCoords[i],
        false,
        bookNames[i],
        bookLore[i],
        opt1[i],
        opt2[i],
        opt3[i],
        correctAnswers[i]
            });
    }
}

void PuzzleManager::Init()
{
    for (int i{}; i < allBooks.size(); ++i) {
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
}
   
void PuzzleManager::OpenExitDoor()
{
    const auto& cfg = GetPuzzleManagerConfig();
    PuzzleManager::activeBookTitle = cfg.exitPopupTitle;
    PuzzleManager::activeBookText = cfg.exitPopupBody;
    PuzzleManager::popupTimer = cfg.exitPopupDuration;
}

} // namespace Game
