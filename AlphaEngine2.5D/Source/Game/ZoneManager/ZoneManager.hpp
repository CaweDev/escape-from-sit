/**
 * @file ZoneManager.hpp
 * @description Header for the ZoneManager system that handles spatial partitioning of the map into zones.
 */
#pragma once
#include <vector>

namespace Engine { class TileMap; }

namespace Game {

    /**
     * @class ZoneManager
     * @description Handles map zone logic and random position generation within zones.
     */
    class ZoneManager
    {
    public:
        void Init(Engine::TileMap* map, int w, int h);
        void SetZoneGrid(const std::vector<int>& grid);

        // returns -1 if out of bounds / not ready
        int GetZoneId(int x, int y) const;

        // returns false if no walkable found
        bool PickRandomWalkableInZone(int zoneId, int& outX, int& outY) const;

        const std::vector<int>& GetGrid() const { return m_zoneGrid; }
        int W() const { return m_w; }
        int H() const { return m_h; }

    private:
        bool InBounds(int x, int y) const { return x >= 0 && y >= 0 && x < m_w && y < m_h; }

    private:
        Engine::TileMap* m_map = nullptr;
        int m_w = 0, m_h = 0;
        std::vector<int> m_zoneGrid;
    };

} // namespace Game