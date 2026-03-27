#include "Heatmap.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <algorithm>

namespace
{
    struct HeatmapGameConfig : public Engine::IConfig
    {
        float decayMin = 0.0f;
        float decayMax = 1.0f;
        float cellMinValue = 0.0f;

        void FromJson(const Json::Value& j) override
        {
            decayMin = j.get("decayMin", decayMin).asFloat();
            decayMax = j.get("decayMax", decayMax).asFloat();
            cellMinValue = j.get("cellMinValue", cellMinValue).asFloat();
        }
    };

    HeatmapGameConfig g_heatmapGameConfig;
    bool g_heatmapGameConfigLoaded = false;

    const HeatmapGameConfig& GetHeatmapGameConfig()
    {
        if (!g_heatmapGameConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/heatmap_game.json", g_heatmapGameConfig);
            if (g_heatmapGameConfig.decayMin > g_heatmapGameConfig.decayMax)
            {
                float t = g_heatmapGameConfig.decayMin;
                g_heatmapGameConfig.decayMin = g_heatmapGameConfig.decayMax;
                g_heatmapGameConfig.decayMax = t;
            }
            g_heatmapGameConfigLoaded = true;
        }
        return g_heatmapGameConfig;
    }
}

namespace Game {

    void Heatmap::Init(int w, int h)
    {
        m_w = w;
        m_h = h;
        m_cells.assign((size_t)m_w * (size_t)m_h, 0.0f);
    }

    void Heatmap::Clear()
    {
        std::fill(m_cells.begin(), m_cells.end(), GetHeatmapGameConfig().cellMinValue);
    }

    void Heatmap::Decay(float factor)
    {
        const auto& cfg = GetHeatmapGameConfig();
        if (factor < cfg.decayMin) factor = cfg.decayMin;
        if (factor > cfg.decayMax) factor = cfg.decayMax;

        for (float& v : m_cells)
            v *= factor;
    }

    void Heatmap::AddAt(int x, int y, float amt)
    {
        if (!InBounds(x, y)) return;

        float& v = m_cells[(size_t)y * (size_t)m_w + (size_t)x];
        v += amt;
        if (v < GetHeatmapGameConfig().cellMinValue) v = GetHeatmapGameConfig().cellMinValue;
    }

    float Heatmap::Get(int x, int y) const
    {
        if (!InBounds(x, y)) return 0.0f;
        return m_cells[(size_t)y * (size_t)m_w + (size_t)x];
    }

    float Heatmap::SumZone(const std::vector<int>& zoneGrid, int zoneId) const
    {
        if ((int)zoneGrid.size() != m_w * m_h) return 0.0f;

        float sum = 0.0f;
        for (int i = 0; i < m_w * m_h; ++i)
            if (zoneGrid[i] == zoneId)
                sum += m_cells[(size_t)i];

        return sum;
    }

    float Heatmap::MaxValue() const
    {
        float mx = 0.0f;
        for (float v : m_cells)
            if (v > mx) mx = v;
        return mx;
    }

    float Heatmap::Total() const
    {
        float sum = 0.0f;
        for (float v : m_cells) sum += v;
        return sum;
    }

} // namespace Game