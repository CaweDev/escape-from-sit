/*!
 * \file Heatmap.hpp
 * \brief Header for the Heatmap used to track activity on the tilemap
 */
#pragma once
#include <vector>

namespace Game {

    /*!
     * \class Heatmap
     * \brief Grid-based activity tracker used by AI and analytics
     */
    class Heatmap
    {
    public:
        /*! \brief Initialize heatmap with width and height. \param w [in] width \param h [in] height */
        void Init(int w, int h);

        /*! \brief Set all cells to zero. */
        void Clear();
        /*! \brief Apply multiplicative decay to all cells. \param factor [in] decay factor in [0..1] */
        void Decay(float factor); // factor in [0..1], e.g. 0.995f

        /*! \brief Add amount to a cell. \param x [in] tile x \param y [in] tile y \param amt [in] amount */
        void AddAt(int x, int y, float amt);
        /*! \brief Get cell value. \param x [in] tile x \param y [in] tile y \return value */
        float Get(int x, int y) const;

        /*! \brief Sum values for a zone using zoneGrid. */
        float SumZone(const std::vector<int>& zoneGrid, int zoneId) const;
        /*! \brief Maximum cell value in heatmap. */
        float MaxValue() const;
        /*! \brief Total of all cells. */
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
