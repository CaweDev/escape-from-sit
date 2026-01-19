#include "PuzzleManager.h"

std::vector<Book> allBooks;
AEGfxVertexList* BookMesh = nullptr;
AEGfxTexture* HLP1 = nullptr;
AEGfxTexture* HLP2 = nullptr;
AEGfxTexture* HLP3 = nullptr;

void PuzzleManager::CreatePuzzleObject()
{
}


void PuzzleManager ::Init()
{
    BookMesh = CreateMesh();
    AEGfxTexture* BookTextures[3];
    BookTextures[0] = AEGfxTextureLoad("Assets/book.png");
    BookTextures[1] = AEGfxTextureLoad("Assets/book.png");
    BookTextures[2] = AEGfxTextureLoad("Assets/book.png");
   
    std::string BookText[3] = { {"Book1"},{"Book2"},{"Book3"} };

    Engine::Vector3 BookCoord[3] = { {10.0f, 0.0f, 5.0f} ,{10.0f, 0.0f, 5.0f},{10.0f, 0.0f, 5.0f} };

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

   