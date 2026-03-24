#pragma once

#include "Engine/Core/Component.hpp"
#include "Engine/UI/UI.hpp"
#include "Game/UCB1Bandit/UCB1Bandit.hpp"
#include "Game/SecurityGuard/SecurityGuard.hpp"
#include "Engine/Graphics/SpriteSheetRenderer.hpp"
#include <vector>
#include "Game/Heatmap/Heatmap.hpp"
namespace Game {

using namespace Engine;
using namespace UI;



class BanditVisualizer : public Component {
public:
    UCB1Bandit* bandit = nullptr; // set by scene

    // Optional pointer to scene guards so we can tint the selected guard
    std::vector<Game::SecurityGuard*>* guards = nullptr;

    // How often to refresh UI (seconds)
    float updateInterval = 0.25f;

    BanditVisualizer();
    ~BanditVisualizer() override;

    void Initialize() override;
    void Update() override;
    void OnDestroy() override;

    void SetGuardsPointer(std::vector<Game::SecurityGuard*>* g) { guards = g; }
    void SetSelectedZone(int zid); // 1..4
    void ToggleVisibility();
    void SetVisible(bool v);
    void SetCCTVScorePointer(float* p) { cctvScores = p; }
    /**
     * Populate the ML debug summary in the visualizer.
     * pulls: 4-length int array
     * means: 4-length float array
     * cctv: 5-length float array (index 1..4 used)
     * heatZones: 5-length float array (index 1..4 used)
     */
    void SetMLSummary(int priorityZone, float hotTimer, bool fullEscalation,
                      const int pulls[4], const float means[4], const float cctv[5], const float heatZones[5]);
    void SetHeatmapPointer(Heatmap* h) { m_heatmap = h; }
    void SetZoneGridPointer(std::vector<int>* g) { m_zoneGrid = g; }

private:
    UI::Canvas* m_canvas = nullptr;
    UI::Panel*  m_panel = nullptr;
    UI::Text*   m_texts[9] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    UI::Text*   m_summary = nullptr;
    UI::Text*   m_priorityText = nullptr;
    UI::Text*   m_cctvText = nullptr;
    UI::Text*   m_heatTexts[4] = { nullptr, nullptr, nullptr, nullptr };
    float*      cctvScores = nullptr; // pointer to scene cctvScore array (1..4)
    int         m_selectedZone = -1;
    float       m_timer = 0.0f;
    // Optional pointers provided by scene for richer debug info
    Heatmap* m_heatmap = nullptr;
    std::vector<int>* m_zoneGrid = nullptr;

    void RefreshUI();

};

} // namespace Game
