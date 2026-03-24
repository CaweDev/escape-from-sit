#pragma once
#include "Engine/Core/Component.hpp"
#include "Engine/UI/UI.hpp"
#include "Game/UCB1Bandit/UCB1Bandit.hpp"
#include "Game/SecurityGuard/SecurityGuard.hpp"
#include "Engine/Graphics/SpriteSheetRenderer.hpp"
#include <vector>
#include <deque>
#include <string>
#include "Game/Heatmap/Heatmap.hpp"

namespace Game {
    using namespace Engine;
    using namespace UI;

    struct CCTVRecord {
        float  timestamp = 0.0f;
        float  posX = 0.0f;
        float  posY = 0.0f;
        int    zone = 0;
        bool   lostSight = false;
        bool   reDetected = false;
    };

    struct ZoneBreakdown {
        float exploitation = 0.0f;
        float exploration = 0.0f;
        float heatInfluence = 0.0f;
        float cctvInfluence = 0.0f;
        float finalScore = 0.0f;
    };

    class BanditVisualizer : public Component {
    public:
        UCB1Bandit* bandit = nullptr;
        std::vector<Game::SecurityGuard*>* guards = nullptr;
        float                             updateInterval = 0.25f;

        BanditVisualizer();
        ~BanditVisualizer() override;

        void Initialize() override;
        void Update()     override;
        void OnDestroy()  override;

        void SetGuardsPointer(std::vector<Game::SecurityGuard*>* g) { guards = g; }
        void SetCCTVScorePointer(float* p) { cctvScores = p; }
        void SetHeatmapPointer(Heatmap* h) { m_heatmap = h; }
        void SetZoneGridPointer(std::vector<int>* g) { m_zoneGrid = g; }

        void SetSelectedZone(int zid);
        void ToggleVisibility();
        void SetVisible(bool v);

        void SetMLSummary(int priorityZone, float hotTimer, bool fullEscalation,
            const int   pulls[4],
            const float means[4],
            const float cctv[5],
            const float heatZones[5],
            const ZoneBreakdown* breakdown = nullptr,
            const char* reason = nullptr);

        void PushCCTVRecord(const CCTVRecord& rec);
        void SetCCTVStatus(int zone, float posX, float posY, bool visible);

    private:
        UI::Canvas* m_canvas = nullptr;
        UI::Panel* m_panel = nullptr;

        UI::Text* m_headerTitle = nullptr;
        UI::Text* m_selectedZoneText = nullptr;
        UI::Text* m_escalationText = nullptr;
        UI::Text* m_hotTimerText = nullptr;
        UI::Text* m_reasonText = nullptr;

        UI::Panel* m_zoneCards[4] = {};
        UI::Text* m_zonePulls[4] = {};
        UI::Text* m_zoneMeans[4] = {};
        UI::Text* m_zoneHeat[4] = {};
        UI::Panel* m_zoneHeatBar[4] = {};
        UI::Text* m_zoneCCTV[4] = {};
        UI::Panel* m_zoneCCTVBar[4] = {};

        UI::Text* m_breakdownZoneLabel = nullptr;
        UI::Text* m_detailLines[5] = {};

        UI::Text* m_cctvCurrent = nullptr;
        UI::Text* m_cctvCurrentPos = nullptr;
        UI::Text* m_cctvCurrentStatus = nullptr;
        UI::Text* m_cctvEntries[10] = {};

        UI::Text* m_texts[9] = {};
        UI::Text* m_summary = nullptr;
        UI::Text* m_priorityText = nullptr;
        UI::Text* m_cctvText = nullptr;
        UI::Text* m_heatTexts[4] = {};

        float* cctvScores = nullptr;
        Heatmap* m_heatmap = nullptr;
        std::vector<int>* m_zoneGrid = nullptr;

        int   m_selectedZone = -1;
        float m_timer = 0.0f;

        std::deque<CCTVRecord> m_cctvHistory;
        static constexpr int   kMaxCCTVEntries = 10;

        ZoneBreakdown m_lastBreakdown = {};

        // layout metrics (pixels, set during Initialize)
        float m_panelWidth = 0.0f;
        float m_panelHeight = 0.0f;
        float m_headerHeight = 0.0f;
        float m_gridRightAnchor = 0.60f;

        bool  m_lastCCTVVisible = false;
        int   m_lastCCTVZone = 0;
        float m_lastCCTVPosX = 0.0f;
        float m_lastCCTVPosY = 0.0f;
        bool  m_hasCCTVState = false;
        float m_cctvLogCooldown = 0.0f;
        float m_elapsedTime = 0.0f;
        void BuildHeader(UI::Panel* parent);
        void BuildZoneGrid(UI::Panel* parent);
        void BuildBreakdownPanel(UI::Panel* parent);
        void BuildCCTVPanel(UI::Panel* parent);

        void RefreshUI();
        void RefreshZoneCards();
        void RefreshBreakdown();
        void RefreshCCTVLog();
        void UpdateBarFill(UI::Panel* bar, float value, float maxValue);

        static std::string FmtFloat(float v, int decimals = 2);
        static std::string ZoneName(int id);
        static std::string ZoneLetter(int id);
    };

} // namespace Game
