/**
 * @file HeatmapOverlay.hpp
 * @brief UI element that displays heatmap data on a map
 */
#pragma once

#include "Engine/UI/Specialized/Map.hpp"
#include "Game/Heatmap/Heatmap.hpp"
#include "Engine/Graphics/Color.hpp"
#include "AEEngine.h"
#include <vector>

namespace UI
{
    /**
     * @class HeatmapOverlay
     * @brief Renders a colored heatmap overlay on top of a map
     *
     * Displays player activity heatmap data as colored tiles.
     * Supports gradient visualization from cold (low activity) to hot (high activity).
     */
    class HeatmapOverlay : public Map
    {
    private:
        // --- Heatmap Data ---
        Heatmap* m_heatmap = nullptr;
        
        // --- Gradient Colors ---
        Engine::Color m_coldColor;   // Color for low heat (e.g., blue)
        Engine::Color m_warmColor;   // Color for medium heat (e.g., yellow)
        Engine::Color m_hotColor;    // Color for high heat (e.g., red)
        
        // --- Display Settings ---
        float m_minThreshold = 0.5f;  // Minimum heat value to display
        float m_maxValue = 100.0f;    // Maximum heat value for color scaling
        float m_opacity = 0.7f;       // Opacity of heatmap overlay (0-1)
        
    public:
        HeatmapOverlay();
        virtual ~HeatmapOverlay();

        /**
         * @brief Render the map with heatmap overlay
         */
        void Render() override;

        /**
         * @brief Set the heatmap data source
         * @param heatmap Pointer to the heatmap (not owned)
         */
        void SetHeatmap(Heatmap* heatmap) { m_heatmap = heatmap; }
        Heatmap* GetHeatmap() const { return m_heatmap; }

        /**
         * @brief Set the color gradient for heatmap visualization
         * @param cold Color for low heat values
         * @param warm Color for medium heat values
         * @param hot Color for high heat values
         */
        void SetHeatGradient(const Engine::Color& cold, const Engine::Color& warm, const Engine::Color& hot);

        /**
         * @brief Set the minimum heat threshold to display
         * @param threshold Minimum value to show (values below are transparent)
         */
        void SetMinThreshold(float threshold) { m_minThreshold = threshold; }
        float GetMinThreshold() const { return m_minThreshold; }

        /**
         * @brief Set the maximum heat value for color scaling
         * @param maxValue Maximum expected heat value
         */
        void SetMaxValue(float maxValue) { m_maxValue = maxValue; }
        float GetMaxValue() const { return m_maxValue; }

        /**
         * @brief Set the opacity of the heatmap overlay
         * @param opacity Opacity value (0.0 = transparent, 1.0 = opaque)
         */
        void SetOpacity(float opacity) { m_opacity = opacity > 1.0f ? 1.0f : (opacity < 0.0f ? 0.0f : opacity); }
        float GetOpacity() const { return m_opacity; }

    private:
        /**
         * @brief Get interpolated color based on heat value
         * @param heatValue Current heat value
         * @return Interpolated color from the gradient
         */
        Engine::Color GetHeatColor(float heatValue) const;
        
        /**
         * @brief Lerp between two colors
         */
        Engine::Color LerpColor(const Engine::Color& a, const Engine::Color& b, float t) const;
    };

} // namespace UI
