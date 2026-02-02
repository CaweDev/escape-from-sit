#include"PuzzleObject.h"
#include <vector>

class PuzzleManager
{
public:
	static std::vector<Book> allBooks;
	static AEGfxVertexList* BookMesh;
	static AEGfxTexture* HLP1[3];
	static void CreatePuzzleObject(Book book);
	static void BookDataInit();
	static std::vector<Book>Books;
	static void Init();
	static void CheckBookCollection(Engine::Vector3 playerPos);


};

