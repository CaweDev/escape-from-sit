#include "PuzzleManager.h"
#include "Engine.hpp"
#include "Engine/Audio/ProximityAudio.hpp"
#include <array>

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
    // Create dummy enemy with collider
    auto bookObj = SceneManager::GetActiveScene()->CreateGameObject("Book");
    bookObj->transform->position = book.BookPosition;
    bookObj->transform->rotation = 0.0f;
    bookObj->transform->localScale = Vector2(0.35f, 0.35f);

    // Enemy collider (static body)
    //auto bookRb = bookObj->AddComponent<Rigidbody2D>();
    //bookRb->bodyType = RigidbodyType2D::Static;

    auto bookPuzzleObj = bookObj->AddComponent<PuzzleObject>();
    //auto bookCollider = bookObj->AddComponent<CircleCollider2D>();
    //bookCollider->radius = 0.5f;
    //bookCollider->minHeight = 0.0f;
    //bookCollider->maxHeight = 2.0f;

    // Enemy sprite
    auto bookSR = bookObj->AddComponent<SpriteRenderer>();
    bookSR->SetSprite(AssetManager::GetSprite("Assets/TempBook1.png"));
    bookSR->billboard = true;
    bookSR->enabled = true;
    bookSR->sortingOrder = 100;


    auto bookInteract = bookObj->AddComponent<Interactable>();
    bookInteract->actionText = "Pick up book";
    bookInteract->maxDistance = 2.5f;
 
    bookInteract->onInteract = [bookPuzzleObj, bookObj, book]() {
        // Only open the quiz if they aren't already reading or answering one!
        if (!bookPuzzleObj->isReading && !PuzzleManager::isQuizActive) {

            // 1. Mark this specific book as currently being read
            bookPuzzleObj->isReading = true;

            // 2. Lock the engine into "Quiz Mode"
            PuzzleManager::isQuizActive = true;

            // 3. Store the data so the HUD knows which book to destroy if they are right
            PuzzleManager::currentBookObj = bookObj;
            PuzzleManager::currentBookId = book.BookId;
            PuzzleManager::currentCorrectAnswer = book.CorrectAnswer;

            // 4. Send the Question (Lore) to the UI
            PuzzleManager::activeBookTitle = book.BookName;
            PuzzleManager::activeBookText = book.BookLore;

            PuzzleManager::activeOpt1 = book.Opt1;
            PuzzleManager::activeOpt2 = book.Opt2;
            PuzzleManager::activeOpt3 = book.Opt3;

            // 5. Set timer to 999 seconds so it stays on screen until they press 1, 2, or 3
            PuzzleManager::popupTimer = 999.0f;
        }
        };

}

void PuzzleManager::BookDataInit()
{
    std::array<std::string, 6> BookName = { "POP QUIZ 1", "POP QUIZ 2", "POP QUIZ 3", "POP QUIZ 4", "POP QUIZ 5", "POP QUIZ 6" };

    // Write your questions and the 3 options inside the Lore string
    std::array<std::string, 6> BookLore = {
        "What does 'int*' represent?",
        "What is a memory leak?",
        "Which of these is a Loop?",
        "|Cinema|",
        "Question 5?",
        "Question 6?"
    };

    std::array<std::string, 6> Opt1 = { "1. A memory address", "1. Spilled RAM", "1. if()", "1. Absolute Cinema", "1. A", "1. A" };
    std::array<std::string, 6> Opt2 = { "2. A decimal number", "2. Unfreed memory", "2. switch()", "2. ???", "2. B", "2. B" };
    std::array<std::string, 6> Opt3 = { "3. A boolean", "3. A fast CPU", "3. while()", "3. Cinema?", "3. C", "3. C" };

    // The correct key they need to press (1, 2, or 3)
    int CorrectAnswers[6] = { 1, 2, 3, 1, 1, 3 };



    //std::array<std::string,6>BookName = { "HLP LESSON 1 ","HLP LESSON 2","HLP LESSON 3","HLP LESSON 4","HLP LESSON 5","HLP LESSON 6" };
    //std::array<std::string, 6> BookLore = {
    //    "GG HLP Mid Term F",
    //    "Need to Lock in for Finals",
    //    "Class template???????",
    //    "Ok maybe can pass now",
    //    "B for HLP!?!?",
    //    "Confirm A now" };
    Engine::Vector3 BookCoord[6] = { {8.0f, 17.0f, 0.3f} ,{9.0f, 41.0f, 0.3f},{90.f, 40.f, 0.3f},{121.0f, 19.f, 0.3f},{83.0f, 12.0f, 0.3f},{73.0f, 19.0f, 0.3f} };

    for (int i{}; i < 6; ++i) {
        allBooks.push_back({
        i,                  // ID
        BookCoord[i],// Position (x, y, z)
        false,              // isCollected  
        BookName[i],
        BookLore[i],
        Opt1[i],
        Opt2[i],
        Opt3[i],
        CorrectAnswers[i]
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
    
    PuzzleManager::activeBookTitle = "SCHOOL GATE UNLOCKED";
    PuzzleManager::activeBookText = "Faster go home lah";
    PuzzleManager::popupTimer = 5.0f;
}

