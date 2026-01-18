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
	bool BookCollected;
	AEGfxVertexList* BookMesh;
	AEGfxTexture *BookTexture;
	std::string BookText;
};


void BookInit();
void BookUpdate();
void BookExit();
AEGfxVertexList* CreateMesh();

