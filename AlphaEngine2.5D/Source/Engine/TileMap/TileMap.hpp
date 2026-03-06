/**
 * @file TileMap.hpp
 * @brief 2D tile map for raycasting walls
 */
#pragma once
#include <vector>
#include "AEEngine.h"
#include "../Core/Component.hpp"
#include "../Math/Vector2.hpp"

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
 * Positive value 10 = door marker (spawns Door, then becomes Empty)
 * Positive value 11 = terminal marker (spawns CCTVTerminal, then becomes Empty)
 * Positive value 12 = exit marker (marked as floor, special gameplay handling)
 * Positive value 13 = control room door marker (spawns ControlRoomDoor, then becomes Empty)
 * Positive values 15-18 = access card markers (Green, Purple, Red, Yellow)
 * Positive value 20 = dedicated control room door tile (easy to place in editor)
 */
enum class TileType : int {
    Empty = 0,   // Default walkable
    Wall = 1,    // Wall type 1
    Wall2 = 2,   // Wall type 2
    Wall3 = 3,   // Wall type 3
    Wall4 = 4,   // Wall type 4
    NUM_WALL = 5,
    // Door marker
    Door = 10,
    NUM_DOOR = 11,
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

    void Initialize() override;

    /** Create map from 2D array */
    void LoadFromArray(const int* data, int w, int h);

    /** Get tile at position */
    TileType GetTile(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            return TileType::Wall;  // Out of bounds = wall
        }
        return tiles[y * width + x];  // Direct indexing - no Y inversion
    }

    /** Set tile at position */
    void SetTile(int x, int y, TileType type) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            tiles[y * width + x] = type;  // Direct indexing - no Y inversion
        }
    }

    /** Check if position is a wall (tile values 1-9 are walls) */
    bool IsWall(int x, int y) const {
        int val = static_cast<int>(GetTile(x, y));
        return val >= static_cast<int>(TileType::Wall) && val < static_cast<int>(TileType::NUM_WALL); // 1-9 are walls, 10+ are door markers
    }

    /** Check if world position is walkable (zero or negative tile values) */
    bool IsWalkable(f32 worldX, f32 worldY) const {
        int tileX = static_cast<int>(worldX);
        int tileY = static_cast<int>(worldY);
        return !IsWall(tileX, tileY);
    }
    
    /** Check if a tile type is a door marker (ONLY tile value 10 = 'D') */
    static bool IsDoorType(TileType type) {
        int val = static_cast<int>(type);
        return IsDoorType(val);
    }
    
    /** Check if a tile type is a door marker (by int) (ONLY tile value 10 = 'D') */
    static bool IsDoorType(int type) {
        return type >= static_cast<int>(TileType::Door) && type < static_cast<int>(TileType::NUM_DOOR);
    }
    
    /** Check if a tile type is a terminal marker */
    static bool IsTerminalMarker(int type) {
        return type == 11; // 'T' in map.txt
    }

    /** Check if a tile type is an exit marker */
    static bool IsExitMarker(int type) {
        return type == 12; // 'E' in map.txt
    }
    
    /** Check if a tile type is a control room door marker */
    static bool IsControlRoomDoorMarker(int type) {
        return type == 13 || type == 20; // Both 'C' (old) and 'K' (new) markers
    }
    
    /** Check if a tile type is an access card marker (cards 0-3) */
    static bool IsAccessCardMarker(int type, int& cardId) {
        if (type >= 15 && type <= 18) {
            cardId = type - 15; // 15=card 0 (Green), 16=card 1 (Purple), 17=card 2 (Red), 18=card 3 (Yellow)
            return true;
        }
        return false;
    }
    
    /** Get all spawned doors */
    const std::vector<Door*>& GetDoors() const { return spawnedDoors; }
    
    /** Get all terminal positions from map */
    const std::vector<Vector2>& GetTerminalPositions() const { return terminalPositions; }
    
    /** Get all exit positions from map */
    const std::vector<Vector2>& GetExitPositions() const { return exitPositions; }
    
    /** Get all control room door positions from map */
    const std::vector<Vector2>& GetControlRoomDoorPositions() const { return controlRoomDoorPositions; }
    
    /** Get all access card spawn positions from map (4 entries, one per card type) */
    const std::vector<std::pair<Vector2, int>>& GetAccessCardPositions() const { return accessCardPositions; }

private:
    bool initialized = false;
    std::vector<TileType> tiles;
    
    /** Doors spawned from door tiles during LoadFromArray */
    std::vector<Door*> spawnedDoors;

    /** Terminal positions found in map (tile 11) */
    std::vector<Vector2> terminalPositions;
        
    /** Exit positions found in map (tile 12) */
    std::vector<Vector2> exitPositions;
    
    /** Control room door positions found in map (tile 13) */
    std::vector<Vector2> controlRoomDoorPositions;
    
    /** Access card spawn positions found in map (tiles 15-18), paired with card ID (0-3) */
    std::vector<std::pair<Vector2, int>> accessCardPositions;

    
    /** Spawn all doors present in the tile map array */
    void SpawnDoors();

    /** Spawn a door at the given tile position */
    void SpawnDoor(int x, int y, int doorType);
};

} // namespace Engine
