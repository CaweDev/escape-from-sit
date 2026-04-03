/**
 * @file TileMap.cpp
 * @brief Implementation of TileMap component
 */
#include "TileMap.hpp"
#include "Door.hpp"
#include "../Core/GameObject.hpp"
#include "../Scene/Scene.hpp"
#include "../Graphics/DoorRenderer.hpp"
#include "../Graphics/TileMapRenderer.hpp"
#include "../Physics/BoxCollider2D.hpp"
#include "../Interaction/Interactable.hpp"
#include "../../Game/Puzzle/PuzzleManager.hpp"
#include <string>
using namespace Engine;

void TileMap::Initialize()
{
	initialized = true;
}


void TileMap::LoadFromArray(const int *data, int w, int h)
{
	width = w;
	height = h;
	tiles.resize(w * h);


	// First pass: load all tiles (direct copy - no inversion)
	for (int i = 0; i < w * h; ++i)
	{
		tiles[i] = static_cast<TileType>(data[i]);
	}

}