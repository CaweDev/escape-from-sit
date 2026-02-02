#include "PuzzleManager.h"
#include "Engine.hpp"

using namespace Engine;
AEGfxTexture* PuzzleManager::HLP1[] = {nullptr,nullptr,nullptr};
std::vector<Book> PuzzleManager::allBooks = {};
AEGfxVertexList* PuzzleManager::BookMesh = nullptr;



void PuzzleManager::CreatePuzzleObject(Book book)
{
    // Create dummy enemy with collider
    auto bookObj = SceneManager::GetActiveScene()->CreateGameObject("Book");
    bookObj->transform->position = book.BookPosition;
    bookObj->transform->rotation = 0.0f;
    bookObj->transform->localScale = Vector2(0.8f, 0.8f);

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
    bookSR->SetSprite(AssetManager::GetSprite("Assets/TempBook.png"));
    bookSR->billboard = true;
    bookSR->enabled = true;
    bookSR->sortingOrder = 100;

    auto bookInteract = bookObj->AddComponent<Interactable>();
    bookInteract->actionText = "Pick up book";
    bookInteract->maxDistance = 2.5f;
    bookInteract->onInteract = [bookPuzzleObj, bookObj]() {
        if (!bookPuzzleObj->isReading) {
            bookPuzzleObj->isReading = true;
            bookObj->active = false;
            std::cout << "Book Collected";

        }

        };

}

void PuzzleManager::BookDataInit()
{

    BookMesh = CreateMesh();
    AEGfxTexture* BookTextures[3];
    BookTextures[0] = AEGfxTextureLoad("Assets/TempBook.png");
    BookTextures[1] = AEGfxTextureLoad("Assets/TempBook.png");
    BookTextures[2] = AEGfxTextureLoad("Assets/TempBook.png");

    AssetManager::GetTexture("Assets/UI/crosshair_default.png");

    std::string BookText[3] = { {"Book1"},{"Book2"},{"Book3"} };

    Engine::Vector3 BookCoord[3] = { {3.5f, 3.5f, 0.0f} ,{9.5f, 37.5f, 0.1f},{105.5f, 38.5f, 0.2f} };


    for (int i{}; i < 3; ++i) {
        allBooks.push_back({
        i,                  // ID
        BookCoord[i],// Position (x, y, z)
        false,              // isCollected
        BookMesh,         // Mesh
        BookTextures[i],// Texture
        BookText[i] // Lore Text
            });
    }
}



void PuzzleManager::Init()
{
    BookMesh= CreateMesh();
    for (int i{}; i < allBooks.size(); ++i) {
        CreatePuzzleObject(allBooks[i]);
    }

}
   


