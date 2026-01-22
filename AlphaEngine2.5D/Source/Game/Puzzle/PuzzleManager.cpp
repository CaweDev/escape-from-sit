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
    enemy->transform->localScale = Vector2(0.2f, 0.2f);

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
    enemySprite->Load("Assets/TempBook.png", 500, 500);
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

    Engine::Vector3 BookCoord[3] = { {1.5f, 1.5f, 0.4F} ,{10.0f, 0.0f, 5.0f},{10.0f, 0.0f, 5.0f} };


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
   