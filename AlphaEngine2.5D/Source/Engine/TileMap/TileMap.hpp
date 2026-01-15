/**
 * @file TileMap.hpp
 * @brief 2D tile map for raycasting walls
 */
#pragma once
#include <vector>
#include <string>
#include "AEEngine.h"
#include "../Core/Component.hpp"

namespace Engine {

/**
 * @enum TileType
 * @brief Types of tiles in the map
 */
enum class TileType : int {
    Empty = 0,
    Wall = 1,
    Wall2 = 2,  // Different wall texture
    Wall3 = 3,
    Wall4 = 4
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

    /** Create map from 2D array */
    void LoadFromArray(const int* data, int w, int h) {
        width = w;
        height = h;
        tiles.resize(w * h);
        for (int i = 0; i < w * h; ++i) {
            tiles[i] = static_cast<TileType>(data[i]);
        }
    }

    /** Get tile at position */
    TileType GetTile(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            return TileType::Wall;  // Out of bounds = wall
        }
        return tiles[y * width + x];
    }

    /** Set tile at position */
    void SetTile(int x, int y, TileType type) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            tiles[y * width + x] = type;
        }
    }

    /** Check if position is a wall */
    bool IsWall(int x, int y) const {
        return GetTile(x, y) != TileType::Empty;
    }

    /** Check if world position is walkable */
    bool IsWalkable(f32 worldX, f32 worldY) const {
        int tileX = static_cast<int>(worldX);
        int tileY = static_cast<int>(worldY);
        return !IsWall(tileX, tileY);
    }

private:
    std::vector<TileType> tiles;
};

} // namespace Engine
