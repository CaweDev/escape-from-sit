/**
 * @file TileMap.hpp
 * @brief 2D tile map for raycasting walls
 */
#pragma once
#include <vector>
#include "AEEngine.h"
#include "../Core/Component.hpp"

namespace Engine {


// Forward declarations
class Door;
class GameObject;

/**
 * @enum TileType
 * @brief Types of tiles in the map
 * Negative values = walkable floors with different textures
 * Zero = empty/walkable
 * Positive values 1-9 = walls (not walkable)
 * Positive values 10-13 = door markers (spawns Door, then becomes Empty)
 */
enum class TileType : int {
    Floor4 = -4, // Walkable floor variant 4
    Floor3 = -3, // Walkable floor variant 3
    Floor2 = -2, // Walkable floor variant 2
    Floor1 = -1, // Walkable floor variant 1
    Empty = 0,   // Default walkable
    Wall = 1,    // Wall type 1
    Wall2 = 2,   // Wall type 2
    Wall3 = 3,   // Wall type 3
    Wall4 = 4,   // Wall type 4
    // Door markers (10-13): spawns Door GameObject, then tile becomes Empty
    Door1 = 10,
    Door2 = 11,
    Door3 = 12,
    Door4 = 13
};

/**
 * @class TileMap
 * @brief 2D grid of tiles for raycasting
 */
class TileMap : public Component {
public:

    int width = 0;
    int height = 0;

    TileMap() = default;
    
    TileMap(int w, int h) : width(w), height(h) {
        tiles.resize(w * h, TileType::Empty);
    }

    /** Create map from 2D array (also spawns Door GameObjects for door tiles) */
    void LoadFromArray(const int* data, int w, int h);

    /** Get tile at position */
    TileType GetTile(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            return TileType::Wall;  // Out of bounds = wall
        }
        return tiles[(height - 1 - y) * width + x];
    }

    /** Set tile at position */
    void SetTile(int x, int y, TileType type) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            tiles[(height - 1 - y) * width + x] = type;
        }
    }

    /** Check if position is a wall (tile values 1-9 are walls) */
    bool IsWall(int x, int y) const {
        int val = static_cast<int>(GetTile(x, y));
        return val > 0 && val < 10; // 1-9 are walls, 10+ are door markers
    }

    /** Check if world position is walkable (zero or negative tile values) */
    bool IsWalkable(f32 worldX, f32 worldY) const {
        int tileX = static_cast<int>(worldX);
        int tileY = static_cast<int>(worldY);
        return !IsWall(tileX, tileY);
    }
    
    /** Check if a tile type is a door marker */
    static bool IsDoorType(TileType type) {
        int val = static_cast<int>(type);
        return val >= 10 && val <= 13;
    }
    
    /** Check if a tile type is a door marker (by int) */
    static bool IsDoorType(int type) {
        return type >= 10 && type <= 13;
    }
    
    /** Get all spawned doors */
    const std::vector<Door*>& GetDoors() const { return spawnedDoors; }

private:
    std::vector<TileType> tiles;
    
    /** Doors spawned from door tiles during LoadFromArray */
    std::vector<Door*> spawnedDoors;
    
    /** Spawn a door at the given tile position */
    void SpawnDoor(int x, int y, int doorType);
};

} // namespace Engine
