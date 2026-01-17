#include "Puzzle.h"
std::vector<Book> allBooks;
AEGfxVertexList* BookMesh = nullptr;
AEGfxTexture* HLP1 = nullptr;
AEGfxTexture* HLP2 = nullptr;
AEGfxTexture* HLP3 = nullptr;


AEGfxVertexList* CreateMesh()
{   
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);

    AEGfxTriAdd(
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
   
    return AEGfxMeshEnd();
    
}

void BookInit()
{
    BookMesh = CreateMesh();

    //LOAD TEXTURE FOR BOOK 1 2 3

    allBooks.push_back({
        1,                  // ID
        {10.0f, 0.0f, 5.0f},// Position (x, y, z)
        false,              // isCollected
        BookMesh,         // Mesh
        HLP1,             // Texture
        "Book1" // Lore Text
        });

    allBooks.push_back({
        2,                  // ID
        {10.0f, 0.0f, 5.0f},// Position (x, y, z)
        false,              // isCollected
        BookMesh,         // Mesh
        HLP2,             // Texture
        "Book2" // Lore Text
        });

    allBooks.push_back({
        3,                  // ID
        {10.0f, 0.0f, 5.0f},// Position (x, y, z)
        false,              // isCollected
        BookMesh,         // Mesh
        HLP3,             // Texture
        "Book3" // Lore Text
        });
}

void BookUpdate()
{
}
