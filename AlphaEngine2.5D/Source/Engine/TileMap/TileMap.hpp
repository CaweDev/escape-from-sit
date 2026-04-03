/**
 * @file TileMap.hpp
 * @brief 2D tile map for raycasting walls
 */
#pragma once
#include <vector>
#include "AEEngine.h"
#include "../Core/Component.hpp"
#include "../Math/Vector2.hpp"

namespace Engine
{

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
    enum class TileType : int
    {
        Empty = 0, // Default walkable
        Wall = 1,  // Wall type 1
        Wall2 = 2, // Wall type 2
        Wall3 = 3, // Wall type 3
        Wall4 = 4, // Wall type 4
        NUM_WALL = 5,
        // Door marker
        Door = 10,
        NUM_DOOR = 11,
    };

    /**
     * @class TileMap
     * @brief 2D grid of tiles for raycasting
     */
    class TileMap : public Component
    {
    public:
        int width = 0;
        int height = 0;

        TileMap() = default;

        TileMap(int w, int h) : width(w), height(h)
        {
            tiles.resize(w * h, TileType::Empty);
        }

        void Initialize() override;

        /** Get tile at position */
        TileType GetTile(int x, int y) const
        {
            if (x < 0 || x >= width || y < 0 || y >= height)
            {
                return TileType::Wall; // Out of bounds = wall
            }
            return tiles[y * width + x]; // Direct indexing - no Y inversion
        }

        /** Set tile at position */
        void SetTile(int x, int y, TileType type)
        {
            if (x >= 0 && x < width && y >= 0 && y < height)
            {
                tiles[y * width + x] = type; // Direct indexing - no Y inversion
            }
        }

        /** Check if position is a wall (tile values 1-9 are walls) */
        bool IsWall(int x, int y) const
        {
            int val = static_cast<int>(GetTile(x, y));
            return val >= static_cast<int>(TileType::Wall) && val < static_cast<int>(TileType::NUM_WALL); // 1-9 are walls, 10+ are door markers
        }

        /** Check if world position is walkable (zero or negative tile values) */
        bool IsWalkable(f32 worldX, f32 worldY) const
        {
            int tileX = static_cast<int>(worldX);
            int tileY = static_cast<int>(worldY);
            return !IsWall(tileX, tileY);
        }

        /** Create map from 2D array */
        void LoadFromArray(const int *data, int w, int h);

    private:
        bool initialized = false;
        std::vector<TileType> tiles;


    };

} // namespace Engine
