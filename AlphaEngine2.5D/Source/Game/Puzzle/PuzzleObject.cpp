#include "PuzzleObject.h"
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





void PuzzleObject::Update()
{
   


}
