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
	std::string BookName;
	std::string BookLore;
	std::string Opt1;
	std::string Opt2;
	std::string Opt3;
	int CorrectAnswer;
};

class PuzzleObject :public Engine::Component
{
public:
	bool PuzzleCompleted;
	Book Reference;
	void Update() override;
	bool isReading;
};




