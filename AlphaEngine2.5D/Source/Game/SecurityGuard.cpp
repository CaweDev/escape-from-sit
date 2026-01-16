#include "SecurityGuard.hpp"
#include "Engine/Core/GameObject.hpp"
#include <cmath>

void Game::SecurityGuard::Initialize()
{


}

void Game::SecurityGuard::Update()
{
	if (!transform) {
		return;
	}

	Engine::Vector2 target = goingtonextPoint ? pointB : pointA;
	Engine::Vector2 ;

}
