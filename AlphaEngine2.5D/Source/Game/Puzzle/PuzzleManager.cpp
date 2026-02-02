#include "PuzzleManager.h"
#include "Engine.hpp"

using namespace Engine;
AEGfxTexture* PuzzleManager::HLP1[] = {nullptr,nullptr,nullptr};
std::vector<Book> PuzzleManager::allBooks = {};
AEGfxVertexList* PuzzleManager::BookMesh = nullptr;



void PuzzleManager::CreatePuzzleObject(Book book)
{
    // Create dummy enemy with collider
    auto enemy = SceneManager::GetActiveScene()->CreateGameObject("Book");
    enemy->transform->position = book.BookPosition;
    enemy->transform->rotation = 0.0f;
    enemy->transform->localScale = Vector2(0.8f, 0.8f);

    // Enemy collider (static body)
    auto enemyRb = enemy->AddComponent<Rigidbody2D>();
    enemyRb->bodyType = RigidbodyType2D::Static;

    auto enemyCollider = enemy->AddComponent<CircleCollider2D>();
    enemyCollider->radius = 0.5f;
    enemyCollider->minHeight = 0.0f;
    enemyCollider->maxHeight = 2.0f;

    // Enemy sprite
    auto enemySR = enemy->AddComponent<SpriteRenderer>();
    std::shared_ptr<Sprite> enemySprite = std::make_shared<Sprite>();
    enemySprite->Load("Assets/TempBook.png");
    enemySR->SetSprite(enemySprite);
    enemySR->billboard = true;
    enemySR->enabled = true;
    enemySR->sortingOrder = 100;
}

void PuzzleManager::BookDataInit()
{

    BookMesh = CreateMesh();
    AEGfxTexture* BookTextures[3];
    BookTextures[0] = AEGfxTextureLoad("Assets/TempBook.png");
    BookTextures[1] = AEGfxTextureLoad("Assets/TempBook.png");
    BookTextures[2] = AEGfxTextureLoad("Assets/TempBook.png");

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
   

void PuzzleManager::CheckBookCollection(Engine::Vector3 playerPos) {
    // 1. Check if the player pressed the "Interact" key (e.g., 'E')
    // Replace 'AE_VK_E' with your specific engine's key code if different
    if (AEInputCheckTriggered(AEVK_E)) {

        // 2. Loop through all books to see if we are close to one
        for (int i = 0; i < allBooks.size(); ++i) {
            Book& book = allBooks[i];

            // Skip if already collected
            if (book.BookCollected) continue;

            // 3. Calculate distance
            float dist = GetDistance(playerPos, book.BookPosition);

            // 4. Check if close enough (e.g., within 2.5 units)
            if (dist < 2.5f) {
                // --- COLLECT THE BOOK ---
                book.BookCollected = true;

                // Triggers your UI Popup (assuming you have these variables)
                isReading = true;
                currentPopupText = book.BookText;

                // Optional: Print to console for debugging
                printf("Collected Book ID: %d\n", book.BookId);

                // Stop checking (prevents picking up 2 books at once)
                break;
            }
        }
    }
}