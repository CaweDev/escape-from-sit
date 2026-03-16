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
            bookObj->active = false;
            std::cout << "Book Collected";
            PuzzleManager::booksCollected++;
            
            PuzzleManager::activeBookTitle = book.BookName;
            PuzzleManager::activeBookText = book.BookLore; // Grab the text for this specific book
            PuzzleManager::popupTimer = 3.0f; // Show it for 3 seconds
        }

        };

}

void PuzzleManager::BookDataInit()
{
    std::array<std::string,6>BookName = { "Book1","Book2","Book3","Book4","Book5","Book6" };
    std::array<std::string, 6> BookLore = {
        "GG HLP Mid Term F",
        "Day 3: I hear footsteps echoing in the halls.",
        "Day 7: The walls seem to be shifting.",
        "Day 12: I found a strange key.",
        "Day 20: I think something is hunting me.",
        "Day ???: Do not look behind you." };
    Engine::Vector3 BookCoord[6] = { {3.5f, 3.5f, 0.0f} ,{9.5f, 37.5f, 0.1f},{105.5f, 38.5f, 0.2f},{9.5f, 30.5f, 0.1f},{9.5f, 40.5f, 0.1f},{9.5f, 45.5f, 0.1f} };

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
   


