#pragma once
#include <vector>
#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Math/Vector3.hpp"

namespace Game
{
    struct CCTVInfo
    {
        Engine::Vector3 pos;
        float rot; 
    };

    
    std::vector<CCTVInfo> GenerateCCTVs(
        Engine::TileMap* map,
        int mapWidth,
        int mapHeight,
        int maxCctv = 10,
        int step = 6,
        float minDist = 10.0f,
        float z = 0.85f,
        float eps = 0.15f
    );
}
