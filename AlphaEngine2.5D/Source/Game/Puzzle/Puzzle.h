#include "AEEngine.h"
#include "Engine/Math/Vector3.hpp"
#include <iostream>
#include<vector>
class Puzzle
{

	bool PuzzleCompleted;



};

bool isReading;


struct Book {
	int BookId;
	Engine::Vector3 BookPosition;
	AEGfxVertexList* BookMesh;
	AEGfxTexture BookTexture;
	bool BookCollected;
	std::string BookText;
};

std::vector<Book> allBook;

void BookInit();
AEGfxVertexList* CreateMesh();

