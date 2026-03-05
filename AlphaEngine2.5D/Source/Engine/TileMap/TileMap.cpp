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
#include <string>

namespace Engine {

void TileMap::LoadFromArray(const int* data, int w, int h) {
    width = w;
    height = h;
    tiles.resize(w * h);
    
    // Clear any previously spawned doors and markers
    spawnedDoors.clear();
    terminalPositions.clear();
    exitPositions.clear();
    
    // First pass: load all tiles (direct copy - no inversion)
    for (int i = 0; i < w * h; ++i) {
        tiles[i] = static_cast<TileType>(data[i]);
    }
    
    // Second pass: spawn doors and collect special markers
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int index = y * w + x;  // Direct indexing - no Y inversion
            int tileValue = static_cast<int>(tiles[index]);
            
            if (IsDoorType(tileValue)) {
                SpawnDoor(x, y, tileValue);
                // Replace door tile with empty (door handles itself now)
                tiles[index] = TileType::Empty;
            }
            else if (IsTerminalMarker(tileValue)) {
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
        }
    }
    
    printf("[TileMap] Loaded %dx%d map: %d doors, %d terminals, %d exits\n", 
           w, h, (int)spawnedDoors.size(), (int)terminalPositions.size(), (int)exitPositions.size());
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
    } else {
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
    DoorRenderer* doorRenderer = doorGO->AddComponent<DoorRenderer>();
    TileMapRenderer* tmRenderer = gameObject->GetComponent<TileMapRenderer>();
    if (tmRenderer) {
        doorRenderer->fov = tmRenderer->fov;
        doorRenderer->sprite = tmRenderer->GetWallSprite(1); // Use default wall sprite
    }
    
    // Add Interactable for player interaction
    Interactable* interactable = doorGO->AddComponent<Interactable>();
    interactable->actionText = "Open";
    interactable->maxDistance = 2.5f;
    
    // Connect interactable to door
    door->SetupInteractable(interactable, "Open", "Close");
    
    // Track the spawned door
    spawnedDoors.push_back(door);
}

} // namespace Engine
