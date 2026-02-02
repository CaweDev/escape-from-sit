#include "AEEngine.h"
#include "Engine/Core/Component.hpp"
#include "Engine/Math/Vector3.hpp"
#include <iostream>
#include<vector>
#include"Engine/Graphics/SpriteRenderer.hpp"

struct Book {
	int BookId;
	Engine::Vector3 BookPosition;
	bool BookCollected;
	AEGfxVertexList* BookMesh;
	AEGfxTexture* BookTexture;
	std::string BookText;
};

class PuzzleObject :public Engine::Component
{
public:
	REQUIRE_COMPONENT(Engine::SpriteRenderer);
	bool PuzzleCompleted;
	Book Reference;
	void Update() override;
	bool isReading;

};



void BookInit();
void BookExit();
AEGfxVertexList* CreateMesh();

