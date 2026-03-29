/*!
 * \file CCTVPlacement.hpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief Utilities to generate CCTV placements per zone
 */

#pragma once
#include <vector>
#include "Engine/Math/Vector3.hpp"
#include "Engine/TileMap/TileMap.hpp"

namespace Game {

    /*!
     * \struct CCTVInfo
     * \brief Information describing a placed CCTV (position, rotation, tile)
     */
    struct CCTVInfo {
        Engine::Vector3 pos;
        float rot; // degrees
        int zoneId;
        int tileX;
        int tileY;
    };

    /*!
     * \brief Generate candidate CCTV placements for each zone in the map.
     * \param map [in] TileMap pointer
     * \param zoneGrid [in] Zone id grid (size = w*h)
     * \param w [in] Map width
     * \param h [in] Map height
     * \param perZone [in] Cameras per zone target
     * \param step [in] Sampling step to reduce CPU
     * \param minDist [in] Minimum distance between cameras in same zone
     * \param z [in] Z height for camera placement
     * \param eps [in] Small offset toward walls
     * \return Vector of CCTVInfo placed
     */
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
