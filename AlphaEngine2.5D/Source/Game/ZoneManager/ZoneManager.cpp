#include "ZoneManager.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include <cstdlib> // std::rand

void ZoneManager::Init(Engine::TileMap* map, int w, int h)
{
    m_map = map;
    m_w = w;
    m_h = h;

    if ((int)m_zoneGrid.size() != m_w * m_h)
        m_zoneGrid.assign(m_w * m_h, 0);
}

void ZoneManager::SetZoneGrid(const std::vector<int>& grid)
{
    m_zoneGrid = grid;
}

int ZoneManager::GetZoneId(int x, int y) const
{
    if (!InBounds(x, y)) return -1;
    if ((int)m_zoneGrid.size() != m_w * m_h) return -1;
    return m_zoneGrid[y * m_w + x];
}

bool ZoneManager::PickRandomWalkableInZone(int zoneId, int& outX, int& outY) const
{
    if (!m_map) return false;
    if (m_w <= 0 || m_h <= 0) return false;
    if ((int)m_zoneGrid.size() != m_w * m_h) return false;

    // random tries first
    for (int tries = 0; tries < 400; ++tries)
    {
        int x = std::rand() % m_w;
        int y = std::rand() % m_h;

        if (m_zoneGrid[y * m_w + x] != zoneId) continue;
        if (!m_map->IsWalkable((float)x + 0.5f, (float)y + 0.5f)) continue;

        outX = x;
        outY = y;
        return true;
    }

    // fallback scan so it won't "randomly fail" on sparse zones
    for (int y = 0; y < m_h; ++y)
    {
        for (int x = 0; x < m_w; ++x)
        {
            if (m_zoneGrid[y * m_w + x] != zoneId) continue;
            if (m_map->IsWalkable((float)x + 0.5f, (float)y + 0.5f))
            {
                outX = x;
                outY = y;
                return true;
            }
        }
    }

    return false;
}