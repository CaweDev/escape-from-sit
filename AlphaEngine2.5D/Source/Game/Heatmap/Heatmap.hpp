/**
 * @file Heatmap.hpp
 * @description Header for the Heatmap class used for tracking player and NPC activity across the map.
 */
#pragma once
#include <vector>

namespace Game {

    /**
     * @class Heatmap
     * @description Grid-based activity tracker for AI decision making.
     */
    class Heatmap
    {
    public:
        void Init(int w, int h);

        void Clear();
        void Decay(float factor); // factor in [0..1], e.g. 0.995f

        void AddAt(int x, int y, float amt);
        float Get(int x, int y) const;

        float SumZone(const std::vector<int>& zoneGrid, int zoneId) const;
        float MaxValue() const;
        float Total() const;

        int W() const { return m_w; }
        int H() const { return m_h; }

    private:
        bool InBounds(int x, int y) const { return x >= 0 && y >= 0 && x < m_w && y < m_h; }

    private:
        int m_w = 0, m_h = 0;
        std::vector<float> m_cells;
    };

} // namespace Game