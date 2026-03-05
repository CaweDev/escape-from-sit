#pragma once
#include <vector>
#include "Engine/Math/Vector3.hpp"
#include "Engine/TileMap/TileMap.hpp"

namespace Game {

    struct CCTVInfo {
        Engine::Vector3 pos;
        float rot; // degrees
        int zoneId;
        int tileX;
        int tileY;
    };

    std::vector<CCTVInfo> GenerateCCTVsPerZone(
        Engine::TileMap* map,
        const std::vector<int>& zoneGrid, // size = w*h, values 1..4
        int w,
        int h,
        int perZone,        // e.g. 3 cameras per zone
        int step,           // e.g. 3 (scan every 3 tiles)
        float minDist,      // e.g. 6.0f
        float z,            // e.g. 0.85f
        float eps           // e.g. 0.02f (offset)
    );

}