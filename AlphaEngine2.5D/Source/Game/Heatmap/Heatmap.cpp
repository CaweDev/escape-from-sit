#include "Heatmap.hpp"
#include <algorithm>

void Heatmap::Init(int w, int h)
{
    m_w = w;
    m_h = h;
    m_cells.assign((size_t)m_w * (size_t)m_h, 0.0f);
}

void Heatmap::Clear()
{
    std::fill(m_cells.begin(), m_cells.end(), 0.0f);
}

void Heatmap::Decay(float factor)
{
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;

    for (float& v : m_cells)
        v *= factor;
}

void Heatmap::AddAt(int x, int y, float amt)
{
    if (!InBounds(x, y)) return;

    float& v = m_cells[(size_t)y * (size_t)m_w + (size_t)x];
    v += amt;
    if (v < 0.0f) v = 0.0f;
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