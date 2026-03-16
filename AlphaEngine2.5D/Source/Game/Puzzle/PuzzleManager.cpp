#include "PuzzleManager.h"
#include "Engine.hpp"
#include <array>

using namespace Engine;
std::vector<Book> PuzzleManager::allBooks = {};
int PuzzleManager::booksCollected = 0;

std::string PuzzleManager::activeBookText = "";
std::string PuzzleManager::activeBookTitle = "";
float PuzzleManager::popupTimer = 0.0f;

void PuzzleManager::CreatePuzzleObject(Book book)
{
    // Create dummy enemy with collider
    auto bookObj = SceneManager::GetActiveScene()->CreateGameObject("Book");
    bookObj->transform->position = book.BookPosition;
    bookObj->transform->rotation = 0.0f;
    bookObj->transform->localScale = Vector2(0.35f, 0.35f);

    // Enemy collider (static body)
    auto bookRb = bookObj->AddComponent<Rigidbody2D>();
    bookRb->bodyType = RigidbodyType2D::Static;

    auto bookPuzzleObj = bookObj->AddComponent<PuzzleObject>();
    auto bookCollider = bookObj->AddComponent<CircleCollider2D>();
    bookCollider->radius = 0.5f;
    bookCollider->minHeight = 0.0f;
    bookCollider->maxHeight = 2.0f;

    // Enemy sprite
    auto bookSR = bookObj->AddComponent<SpriteRenderer>();
    bookSR->SetSprite(AssetManager::GetSprite("Assets/TempBook1.png"));
    bookSR->billboard = true;
    bookSR->enabled = true;
    bookSR->sortingOrder = 100;

    auto bookInteract = bookObj->AddComponent<Interactable>();
    bookInteract->actionText = "Pick up book";
    bookInteract->maxDistance = 2.5f;
    bookInteract->onInteract = [bookPuzzleObj, bookObj,book]() {
        if (!bookPuzzleObj->isReading) {
            bookPuzzleObj->isReading = true;

            
            Engine::GameObject::Destroy(bookObj);
            std::cout << "[MEMORY CLEANUP] GameObject "; // TEMPORARY TO TEST IF OBJECT IS DESTROYED
            PuzzleManager::allBooks[book.BookId].BookCollected = true;
            PuzzleManager::booksCollected++;
            
            PuzzleManager::activeBookTitle = book.BookName;
            PuzzleManager::activeBookText = book.BookLore; // Grab the text for this specific book
            PuzzleManager::popupTimer = 3.0f; // Show it for 3 seconds

            if (PuzzleManager::booksCollected >= 6) {
                PuzzleManager::OpenExitDoor();
            }
           
        }

        };

}

void PuzzleManager::BookDataInit()
{
    std::array<std::string,6>BookName = { "HLP LESSON 1 ","HLP LESSON 2","HLP LESSON 3","HLP LESSON 4","HLP LESSON 5","HLP LESSON 6" };
    std::array<std::string, 6> BookLore = {
        "GG HLP Mid Term F",
        "Need to Lock in for Finals",
        "Class template???????",
        "Ok maybe can pass now",
        "B for HLP!?!?",
        "Confirm A now" };
    Engine::Vector3 BookCoord[6] = { {8.0f, 17.0f, 0.3f} ,{9.0f, 41.0f, 0.3f},{90.f, 40.f, 0.3f},{121.0f, 19.f, 0.3f},{83.0f, 12.0f, 0.3f},{73.0f, 19.0f, 0.3f} };

    for (int i{}; i < 6; ++i) {
        allBooks.push_back({
        i,                  // ID
        BookCoord[i],// Position (x, y, z)
        false,              // isCollected  
        BookName[i],
        BookLore[i]
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

