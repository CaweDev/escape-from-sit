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
#include "../../Game/Puzzle/PuzzleManager.h"
#include <string>
using namespace Engine;

void TileMap::Initialize()
{
	initialized = true;
	SpawnDoors();
}

void TileMap::AddExitAt(int x, int y) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    Vector2 pos(static_cast<f32>(x) + 0.5f, static_cast<f32>(y) + 0.5f);
    // Avoid duplicates
    for (auto& p : exitPositions) {
        if ((int)std::floor(p.x) == x && (int)std::floor(p.y) == y) return;
    }
    exitPositions.push_back(pos);
    // If the TileMap is already initialized, spawn a door gameobject for this exit immediately
    if (initialized) {
        SpawnDoor(x, y, 12);
    }
}

void TileMap::RemoveExitAt(int x, int y) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    exitPositions.erase(std::remove_if(exitPositions.begin(), exitPositions.end(), [x,y](const Vector2& p){
        return (int)std::floor(p.x) == x && (int)std::floor(p.y) == y;
    }), exitPositions.end());

    // Also destroy any spawned door corresponding to this exit (doorType 12)
    for (auto it = spawnedDoors.begin(); it != spawnedDoors.end();) {
        Door* d = *it;
        if (d && d->tileX == x && d->tileY == y && d->doorType == 12) {
            if (d->gameObject) {
                GameObject::Destroy(d->gameObject);
            }
            it = spawnedDoors.erase(it);
        } else {
            ++it;
        }
    }
}

void TileMap::LoadFromArray(const int* data, int w, int h) {
    width = w;
    height = h;
    tiles.resize(w * h);
    
    // Clear any previously spawned doors and markers
    spawnedDoors.clear();
    terminalPositions.clear();
    exitPositions.clear();
    controlRoomDoorPositions.clear();
    accessCardPositions.clear();
    
    // First pass: load all tiles (direct copy - no inversion)
    for (int i = 0; i < w * h; ++i) {
        tiles[i] = static_cast<TileType>(data[i]);
    }
    
    // Second pass: spawn doors and collect special markers
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int index = y * w + x;  // Direct indexing - no Y inversion
            int tileValue = static_cast<int>(tiles[index]);
            
          	if (IsTerminalMarker(tileValue)) {
                // Store terminal position (center of tile)
                terminalPositions.push_back(Vector2(static_cast<f32>(x) + 0.5f, static_cast<f32>(y) + 0.5f));
                // Replace terminal marker with floor
                tiles[index] = TileType::Empty;
                printf("[TileMap] Found terminal marker at (%d, %d)\n", x, y);
            }
            else if (IsExitMarker(tileValue)) {
                // Store exit position (center of tile)
                exitPositions.push_back(Vector2(static_cast<f32>(x) + 0.5f, static_cast<f32>(y) + 0.5f));
                // Replace exit marker with floor
                tiles[index] = TileType::Empty;
                printf("[TileMap] Found exit marker at (%d, %d)\n", x, y);
            }
            else if (IsControlRoomDoorMarker(tileValue)) {
                // Store control room door position (center of tile)
                controlRoomDoorPositions.push_back(Vector2(static_cast<f32>(x) + 0.5f, static_cast<f32>(y) + 0.5f));
                // Replace marker with floor (door will be spawned by FPSScene)
                tiles[index] = TileType::Empty;
                printf("[TileMap] Found control room door marker at (%d, %d)\n", x, y);
            }
            else {
                // Check for access card markers
                int cardId = -1;
                if (IsAccessCardMarker(tileValue, cardId)) {
                    // Store access card spawn position with card ID
                    accessCardPositions.push_back(std::make_pair(
                        Vector2(static_cast<f32>(x) + 0.5f, static_cast<f32>(y) + 0.5f),
                        cardId
                    ));
                    // Replace marker with floor (card will be spawned by FPSScene)
                    tiles[index] = TileType::Empty;
                    const char* cardNames[] = {"Green", "Purple", "Red", "Yellow"};
                    printf("[TileMap] Found %s access card marker at (%d, %d)\n", cardNames[cardId], x, y);
                }
            }
        }
    }
    
    printf("[TileMap] Loaded %dx%d map: %d doors, %d terminals, %d exits, %d control room doors, %d access cards\n", 
           w, h, (int)spawnedDoors.size(), (int)terminalPositions.size(), (int)exitPositions.size(),
           (int)controlRoomDoorPositions.size(), (int)accessCardPositions.size());

	if (initialized) {
		SpawnDoors();
	}

	printf("[TileMap] Loaded %dx%d map: %d doors, %d terminals, %d exits\n",
		w, h, (int)spawnedDoors.size(), (int)terminalPositions.size(), (int)exitPositions.size());
}

void TileMap::SpawnDoors() {
	// Destroy existing door GameObjects
	for (Door* door : spawnedDoors) {
		if (door && door->gameObject) {
			GameObject::Destroy(door->gameObject);
		}
	}
	spawnedDoors.clear();

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int index = y * width + x;
			int tileValue = static_cast<int>(tiles[index]);

			if (IsDoorType(tileValue)) {
				SpawnDoor(x, y, tileValue);
				// Replace door tile with empty (door handles itself)
				tiles[index] = TileType::Empty;
			}
		}
	}

	// build spawnedDoorPositions cache
	spawnedDoorPositions.clear();
	for (Door* d : spawnedDoors) {
		if (!d) continue;
		if (d->gameObject) {
			spawnedDoorPositions.push_back(Vector2(static_cast<f32>(d->tileX) + 0.5f, static_cast<f32>(d->tileY) + 0.5f));
		}
	}

	// Spawn doors for exit markers that were recorded during LoadFromArray
	for (const Vector2& p : exitPositions) {
		int x = (int)std::floor(p.x);
		int y = (int)std::floor(p.y);
		// Spawn an exit door (use doorType 12) if within bounds
		if (x >= 0 && x < width && y >= 0 && y < height) {
			SpawnDoor(x, y, 12);
		}
	}

}

void TileMap::SpawnDoor(int x, int y, int doorType) {
	if (!gameObject) return;

	// Get the scene from our parent GameObject
	Scene* scene = gameObject->scene;
	if (!scene) {
		return;
	}

	// Create door GameObject through the scene so it's properly tracked
	std::string doorName = "Door_" + std::to_string(x) + "_" + std::to_string(y);
	GameObject* doorGO = scene->CreateGameObject(doorName);

	// Set up transform position (center of tile)
	doorGO->transform->position.x = static_cast<f32>(x) + 0.5f;
	doorGO->transform->position.y = static_cast<f32>(y) + 0.5f;

	// Determine facing based on surrounding walls
	bool wallNorth = (y + 1 < height) && IsWall(x, y + 1);
	bool wallSouth = (y - 1 >= 0) && IsWall(x, y - 1);

	DoorFacing facing;
	if (wallNorth || wallSouth) {
		facing = DoorFacing::EastWest;  // Door blocks N/S passage if walls are to the N/S?? 
		// Wait, standard convention: if walls are North and South, the door is between them, blocking East-West?
		// Let's check: if there are walls at N and S, the opening is between them. 
		// So the door segment should run N-S.
		// But wait, the DoorRenderer logic:
		// DoorFacing::NorthSouth => Runs East-West (constant Y)
		// DoorFacing::EastWest => Runs North-South (constant X)
		// So if walls are North and South, we want a door that runs North-South, which is EastWest facing.
		facing = DoorFacing::EastWest;
	}
	else {
		facing = DoorFacing::NorthSouth;
	}

	// Add BoxCollider2D (Door::Awake will refine size)
	BoxCollider2D* collider = doorGO->AddComponent<BoxCollider2D>();
	collider->minHeight = 0.0f;
	collider->maxHeight = 2.0f;
	collider->isTrigger = false;

	// Add Door component
	Door* door = doorGO->AddComponent<Door>();
	door->tileX = x;
	door->tileY = y;
	door->doorType = doorType;
	door->facing = facing;
	door->collider = collider;

	// Add DoorRenderer

	TileMapRenderer* tmRenderer = gameObject->GetComponent<TileMapRenderer>();
	if (tmRenderer) {
		DoorRenderer* doorRenderer = doorGO->AddComponent<DoorRenderer>();
		doorRenderer->fov = tmRenderer->fov;
		doorRenderer->sprite = tmRenderer->GetDoorSprite(); // Use default wall sprite
	}

	// Add Interactable for player interaction
	Interactable* interactable = doorGO->AddComponent<Interactable>();
	interactable->actionText = "Open";
	interactable->maxDistance = 2.5f;

	// Connect interactable to door
	door->SetupInteractable(interactable, "Open", "Close");

    // If this is an exit door (doorType 12) lock it until all puzzle books are collected
    if (doorType == 12) {
        int totalBooks = (int)PuzzleManager::allBooks.size();

        // Initial hover text
        if (PuzzleManager::booksCollected < totalBooks) {
            interactable->actionText = "EXIT LOCKED: Collect all books";
        }

        // Override interaction to enforce lock
        // Notice we capture `interactable` here so we can change the text later!
        interactable->onInteract = [door, totalBooks, interactable]() {

            if (PuzzleManager::booksCollected >= totalBooks) {
                // 1. Physically open the door in the world
                door->Toggle();

                // 2. Fix the hover text so it no longer says "Locked"
                interactable->actionText = "Close";

                // 3. Trigger your global win state and UI
                PuzzleManager::OpenExitDoor();

                printf("[TileMap] Exit door opened (all books collected)\n");
            }
            else {
                int remaining =PuzzleManager::booksCollected;

                // NEW: Route the locked message to your HUD so the player actually sees it!
                PuzzleManager::activeBookTitle = "DOOR LOCKED";
                PuzzleManager::activeBookText = std::to_string(remaining) + "/6 books collected";
                PuzzleManager::popupTimer = 3.0f;

                printf("[TileMap] Exit is locked. %d books remaining.\n", remaining);
            }
        };
	}

	// Track the spawned door
	spawnedDoors.push_back(door);
}
