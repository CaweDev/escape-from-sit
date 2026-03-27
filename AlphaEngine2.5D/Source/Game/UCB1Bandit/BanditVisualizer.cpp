#include "BanditVisualizer.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include <cmath>

namespace
{
    struct BanditVisualizerConfig : public Engine::IConfig
    {
        float dtFallback = 0.016f;
        float panelWidthScale = 0.78f;
        float panelHeightScale = 0.78f;
        float headerHeight = 92.0f;
        float maxHeat = 50.0f;
        float maxCCTV = 6.0f;
        float barWidth = 120.0f;

        void FromJson(const Json::Value& j) override
        {
            dtFallback = j.get("dtFallback", dtFallback).asFloat();
            panelWidthScale = j.get("panelWidthScale", panelWidthScale).asFloat();
            panelHeightScale = j.get("panelHeightScale", panelHeightScale).asFloat();
            headerHeight = j.get("headerHeight", headerHeight).asFloat();
            maxHeat = j.get("maxHeat", maxHeat).asFloat();
            maxCCTV = j.get("maxCCTV", maxCCTV).asFloat();
            barWidth = j.get("barWidth", barWidth).asFloat();
        }
    };

    BanditVisualizerConfig g_banditVisConfig;
    bool g_banditVisConfigLoaded = false;

    const BanditVisualizerConfig& GetBanditVisualizerConfig()
    {
        if (!g_banditVisConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/bandit_visualizer.json", g_banditVisConfig);
            if (g_banditVisConfig.dtFallback <= 0.0f) g_banditVisConfig.dtFallback = 0.016f;
            if (g_banditVisConfig.panelWidthScale <= 0.01f) g_banditVisConfig.panelWidthScale = 0.78f;
            if (g_banditVisConfig.panelHeightScale <= 0.01f) g_banditVisConfig.panelHeightScale = 0.78f;
            if (g_banditVisConfig.maxHeat <= 0.01f) g_banditVisConfig.maxHeat = 50.0f;
            if (g_banditVisConfig.maxCCTV <= 0.01f) g_banditVisConfig.maxCCTV = 6.0f;
            if (g_banditVisConfig.barWidth < 0.0f) g_banditVisConfig.barWidth = 0.0f;
            g_banditVisConfigLoaded = true;
        }
        return g_banditVisConfig;
    }
}

namespace Game {

    std::string BanditVisualizer::FmtFloat(float v, int decimals) {
        char buf[64];
        const char* fmt = (decimals == 3) ? "%.3f" : (decimals == 1) ? "%.1f" : "%.2f";
        snprintf(buf, sizeof(buf), fmt, v);
        return std::string(buf);
    }

    std::string BanditVisualizer::ZoneName(int id) {
        if (id < 1 || id > 4) return "Zone ?";
        return std::string("ZONE ") + std::to_string(id);
    }

    std::string BanditVisualizer::ZoneLetter(int id) {
        if (id < 1 || id > 4) return "N/A";
        return std::string("Z") + std::to_string(id);
    }

    BanditVisualizer::BanditVisualizer() {}
    BanditVisualizer::~BanditVisualizer() {}

    void BanditVisualizer::Initialize() {
        const auto& cfg = GetBanditVisualizerConfig();
        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        if (!m_canvas) return;

        int winW = Engine::Application::GetWindowWidth();
        int winH = Engine::Application::GetWindowHeight();
        if (winW <= 0) winW = 1280;
        if (winH <= 0) winH = 720;

        m_canvas->SetReferenceResolution((float)winW, (float)winH);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);
        m_canvas->SetSortOrder(1000);

        m_panel = m_canvas->CreateChild<UI::Panel>();
        m_panel->name = "BanditDashboard";
        m_panel->SetAnchors({ 0.5f, 0.5f }, { 0.5f, 0.5f });
        m_panel->SetPivot({ 0.5f, 0.5f });
        m_panel->SetAnchoredPosition({ 0.0f, 0.0f });

        float panelW = (float)winW * cfg.panelWidthScale;
        float panelH = (float)winH * cfg.panelHeightScale;
        m_panel->SetSizeDelta({ panelW, panelH });
        m_panel->SetBackgroundColor(Engine::Color(0.05f, 0.05f, 0.05f, 0.95f));
        m_panel->visible = false;

        m_panelWidth = panelW;
        m_panelHeight = panelH;
        m_headerHeight = cfg.headerHeight;

        BuildHeader(m_panel);
        BuildBreakdownPanel(m_panel);
        BuildZoneGrid(m_panel);
        BuildCCTVPanel(m_panel);

        m_summary = nullptr;
        m_cctvText = nullptr;
        for (int i = 0; i < 4; ++i) m_heatTexts[i] = nullptr;
        for (int i = 0; i < 9; ++i) m_texts[i] = nullptr;

        m_selectedZone = -1;
        m_timer = 0.0f;

        RefreshUI();
    }

    void BanditVisualizer::BuildHeader(UI::Panel* parent) {
        UI::Panel* hdr = parent->CreateChild<UI::Panel>();
        hdr->name = "Header";
        hdr->SetAnchors({ 0.02f, 1.0f }, { 0.98f, 1.0f });
        hdr->SetPivot({ 0.5f, 1.0f });
        hdr->SetAnchoredPosition({ 0.0f, -10.0f });
        hdr->SetSizeDelta({ 0.0f, m_headerHeight });
        hdr->SetBackgroundColor(Engine::Color(0.07f, 0.07f, 0.07f, 0.95f));

        m_headerTitle = hdr->CreateChild<UI::Text>();
        m_headerTitle->name = "Title";
        m_headerTitle->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        m_headerTitle->SetPivot({ 0.5f, 1.0f });
        m_headerTitle->SetAnchoredPosition({ 0.0f, -8.0f });
        m_headerTitle->SetSizeDelta({ 0.0f, 26.0f });
        m_headerTitle->SetFontSize(1.45f);
        m_headerTitle->SetColor(Engine::Color(0.95f, 0.95f, 1.0f, 1.0f));
        m_headerTitle->SetText("AI / UCB DECISION DASHBOARD");
        m_headerTitle->SetAnchor(UI::TextAnchor::UpperCenter);

        m_selectedZoneText = hdr->CreateChild<UI::Text>();
        m_selectedZoneText->name = "SelectedZone";
        m_selectedZoneText->SetAnchors({ 0.02f, 0.42f }, { 0.32f, 0.72f });
        m_selectedZoneText->SetPivot({ 0.0f, 0.5f });
        m_selectedZoneText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_selectedZoneText->SetSizeDelta({ 0.0f, 0.0f });
        m_selectedZoneText->SetFontSize(1.08f);
        m_selectedZoneText->SetColor(Engine::Color(1, 1, 1, 1));
        m_selectedZoneText->SetText("Selected Zone: N/A");
        m_selectedZoneText->SetAnchor(UI::TextAnchor::MiddleLeft);

        m_escalationText = hdr->CreateChild<UI::Text>();
        m_escalationText->name = "Escalation";
        m_escalationText->SetAnchors({ 0.34f, 0.42f }, { 0.66f, 0.72f });
        m_escalationText->SetPivot({ 0.5f, 0.5f });
        m_escalationText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_escalationText->SetSizeDelta({ 0.0f, 0.0f });
        m_escalationText->SetFontSize(1.08f);
        m_escalationText->SetColor(Engine::Color(1, 1, 1, 1));
        m_escalationText->SetText("Escalation: OFF");
        m_escalationText->SetAnchor(UI::TextAnchor::MiddleCenter);

        m_hotTimerText = hdr->CreateChild<UI::Text>();
        m_hotTimerText->name = "HotTimer";
        m_hotTimerText->SetAnchors({ 0.68f, 0.42f }, { 0.98f, 0.72f });
        m_hotTimerText->SetPivot({ 1.0f, 0.5f });
        m_hotTimerText->SetAnchoredPosition({ 0.0f, 0.0f });
        m_hotTimerText->SetSizeDelta({ 0.0f, 0.0f });
        m_hotTimerText->SetFontSize(1.08f);
        m_hotTimerText->SetColor(Engine::Color(1, 1, 1, 1));
        m_hotTimerText->SetText("Hot Timer: 0.00");
        m_hotTimerText->SetAnchor(UI::TextAnchor::MiddleRight);

        m_reasonText = hdr->CreateChild<UI::Text>();
        m_reasonText->name = "Reason";
        m_reasonText->SetAnchors({ 0.02f, 0.05f }, { 0.98f, 0.32f });
        m_reasonText->SetPivot({ 0.5f, 0.0f });
        m_reasonText->SetAnchoredPosition({ 0.0f, 4.0f });
        m_reasonText->SetSizeDelta({ 0.0f, 0.0f });
        m_reasonText->SetFontSize(0.98f);
        m_reasonText->SetColor(Engine::Color(0.8f, 0.95f, 0.8f, 1.0f));
        m_reasonText->SetText("Reason: N/A");
        m_reasonText->SetAnchor(UI::TextAnchor::MiddleCenter);

        m_priorityText = m_selectedZoneText;
    }

    void BanditVisualizer::BuildBreakdownPanel(UI::Panel* parent) {
        UI::Panel* bp = parent->CreateChild<UI::Panel>();
        bp->name = "BreakdownPanel";
        bp->SetAnchors({ 0.03f, 0.50f }, { 0.67f, 0.86f });
        bp->SetPivot({ 0.5f, 0.5f });
        bp->SetAnchoredPosition({ 0.0f, 0.0f });
        bp->SetSizeDelta({ 0.0f, 0.0f });
        bp->SetBackgroundColor(Engine::Color(0.07f, 0.07f, 0.07f, 0.95f));

        UI::Text* title = bp->CreateChild<UI::Text>();
        title->name = "BPTitle";
        title->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        title->SetPivot({ 0.5f, 1.0f });
        title->SetAnchoredPosition({ 0.0f, -8.0f });
        title->SetSizeDelta({ 0.0f, 22.0f });
        title->SetFontSize(1.1f);
        title->SetColor(Engine::Color(0.85f, 0.85f, 0.85f, 1.0f));
        title->SetText("SELECTED ZONE BREAKDOWN");
        title->SetAnchor(UI::TextAnchor::UpperCenter);

        UI::Panel* sep = bp->CreateChild<UI::Panel>();
        sep->SetAnchors({ 0.03f, 1.0f }, { 0.97f, 1.0f });
        sep->SetPivot({ 0.5f, 1.0f });
        sep->SetAnchoredPosition({ 0.0f, -34.0f });
        sep->SetSizeDelta({ 0.0f, 1.0f });
        sep->SetBackgroundColor(Engine::Color(0.3f, 0.3f, 0.3f, 0.8f));

        m_breakdownZoneLabel = bp->CreateChild<UI::Text>();
        m_breakdownZoneLabel->name = "BPZoneLabel";
        m_breakdownZoneLabel->SetAnchors({ 0.08f, 0.76f }, { 0.92f, 0.86f });
        m_breakdownZoneLabel->SetPivot({ 0.5f, 0.5f });
        m_breakdownZoneLabel->SetAnchoredPosition({ 0.0f, 0.0f });
        m_breakdownZoneLabel->SetSizeDelta({ 0.0f, 0.0f });
        m_breakdownZoneLabel->SetFontSize(1.08f);
        m_breakdownZoneLabel->SetColor(Engine::Color(1.0f, 0.88f, 0.30f, 1.0f));
        m_breakdownZoneLabel->SetText("Zone: N/A");
        m_breakdownZoneLabel->SetAnchor(UI::TextAnchor::MiddleCenter);

        const Engine::Color lineColors[] = {
            Engine::Color(0.85f, 0.85f, 0.85f, 1.0f),
            Engine::Color(0.85f, 0.85f, 0.85f, 1.0f),
            Engine::Color(1.0f, 0.65f, 0.25f, 1.0f),
            Engine::Color(0.40f, 0.75f, 1.0f, 1.0f),
            Engine::Color(1.0f, 0.88f, 0.30f, 1.0f)
        };

        for (int i = 0; i < 5; ++i) {
            UI::Text* line = bp->CreateChild<UI::Text>();
            line->name = std::string("BPLine_") + std::to_string(i);
            float top = 0.58f - i * 0.12f;
            line->SetAnchors({ 0.12f, top }, { 0.88f, top + 0.08f });
            line->SetPivot({ 0.5f, 0.5f });
            line->SetAnchoredPosition({ 0.0f, 0.0f });
            line->SetSizeDelta({ 0.0f, 0.0f });
            line->SetFontSize(1.0f);
            line->SetColor(lineColors[i]);
            line->SetText("---");
            line->SetAnchor(UI::TextAnchor::MiddleCenter);
            m_detailLines[i] = line;
        }
    }
    void BanditVisualizer::BuildZoneGrid(UI::Panel* parent) {
        UI::Panel* grid = parent->CreateChild<UI::Panel>();
        grid->name = "ZoneGrid";
        grid->SetAnchors({ 0.03f, 0.06f }, { 0.67f, 0.46f });
        grid->SetPivot({ 0.5f, 0.5f });
        grid->SetAnchoredPosition({ 0.0f, 0.0f });
        grid->SetSizeDelta({ 0.0f, 0.0f });
        grid->SetBackgroundColor(Engine::Color(0.07f, 0.07f, 0.07f, 0.95f));

        UI::Text* lbl = grid->CreateChild<UI::Text>();
        lbl->name = "ZoneOverviewLabel";
        lbl->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        lbl->SetPivot({ 0.5f, 1.0f });
        lbl->SetAnchoredPosition({ 0.0f, -8.0f });
        lbl->SetSizeDelta({ 0.0f, 22.0f });
        lbl->SetFontSize(1.1f);
        lbl->SetColor(Engine::Color(0.85f, 0.85f, 0.85f, 1.0f));
        lbl->SetText("ZONE OVERVIEW");
        lbl->SetAnchor(UI::TextAnchor::UpperCenter);

        auto MakeCard = [&](int zoneId, AEVec2 aMin, AEVec2 aMax) {
            int idx = zoneId - 1;

            UI::Panel* card = grid->CreateChild<UI::Panel>();
            card->name = std::string("Card_Z") + std::to_string(zoneId);
            card->SetAnchors(aMin, aMax);
            card->SetPivot({ 0.5f, 0.5f });
            card->SetAnchoredPosition({ 0.0f, 0.0f });
            card->SetSizeDelta({ -14.0f, -14.0f });
            card->SetBackgroundColor(Engine::Color(0.09f, 0.09f, 0.09f, 0.95f));
            m_zoneCards[idx] = card;

            UI::Text* title = card->CreateChild<UI::Text>();
            title->name = std::string("ZTitle_") + std::to_string(zoneId);
            title->SetAnchors({ 0.5f, 1.0f }, { 0.5f, 1.0f });
            title->SetPivot({ 0.5f, 1.0f });
            title->SetAnchoredPosition({ 0.0f, -8.0f });
            title->SetSizeDelta({ 140.0f, 20.0f });
            title->SetFontSize(1.05f);
            title->SetColor(Engine::Color(0.95f, 0.95f, 0.95f, 1.0f));
            title->SetText(ZoneName(zoneId).c_str());
            title->SetAnchor(UI::TextAnchor::UpperCenter);

            m_zonePulls[idx] = card->CreateChild<UI::Text>();
            m_zonePulls[idx]->SetAnchors({ 0.0f, 0.0f }, { 0.0f, 0.0f });
            m_zonePulls[idx]->SetPivot({ 0.0f, 0.5f });
            m_zonePulls[idx]->SetAnchoredPosition({ 12.0f, 76.0f });
            m_zonePulls[idx]->SetSizeDelta({ 130.0f, 18.0f });
            m_zonePulls[idx]->SetFontSize(0.92f);
            m_zonePulls[idx]->SetColor(Engine::Color(0.85f, 0.85f, 0.85f, 1.0f));
            m_zonePulls[idx]->SetText("pulls: 0");
            m_zonePulls[idx]->SetAnchor(UI::TextAnchor::MiddleLeft);

            m_zoneMeans[idx] = card->CreateChild<UI::Text>();
            m_zoneMeans[idx]->SetAnchors({ 0.0f, 0.0f }, { 0.0f, 0.0f });
            m_zoneMeans[idx]->SetPivot({ 0.0f, 0.5f });
            m_zoneMeans[idx]->SetAnchoredPosition({ 12.0f, 56.0f });
            m_zoneMeans[idx]->SetSizeDelta({ 130.0f, 18.0f });
            m_zoneMeans[idx]->SetFontSize(0.92f);
            m_zoneMeans[idx]->SetColor(Engine::Color(0.75f, 1.0f, 0.75f, 1.0f));
            m_zoneMeans[idx]->SetText("mean: 0.00");
            m_zoneMeans[idx]->SetAnchor(UI::TextAnchor::MiddleLeft);

            m_zoneHeat[idx] = card->CreateChild<UI::Text>();
            m_zoneHeat[idx]->SetAnchors({ 0.0f, 0.0f }, { 0.0f, 0.0f });
            m_zoneHeat[idx]->SetPivot({ 0.0f, 0.5f });
            m_zoneHeat[idx]->SetAnchoredPosition({ 12.0f, 34.0f });
            m_zoneHeat[idx]->SetSizeDelta({ 50.0f, 18.0f });
            m_zoneHeat[idx]->SetFontSize(0.88f);
            m_zoneHeat[idx]->SetColor(Engine::Color(0.9f, 0.9f, 0.9f, 1.0f));
            m_zoneHeat[idx]->SetText("heat:");
            m_zoneHeat[idx]->SetAnchor(UI::TextAnchor::MiddleLeft);

            UI::Panel* heatBg = card->CreateChild<UI::Panel>();
            heatBg->SetAnchors({ 0.0f, 0.0f }, { 0.0f, 0.0f });
            heatBg->SetPivot({ 0.0f, 0.5f });
            heatBg->SetAnchoredPosition({ 78.0f, 34.0f });
            heatBg->SetSizeDelta({ 120.0f, 10.0f });
            heatBg->SetBackgroundColor(Engine::Color(0.25f, 0.25f, 0.25f, 1.0f));

            UI::Panel* heatFill = heatBg->CreateChild<UI::Panel>();
            heatFill->SetAnchors({ 0.0f, 0.5f }, { 0.0f, 0.5f });
            heatFill->SetPivot({ 0.0f, 0.5f });
            heatFill->SetAnchoredPosition({ 0.0f, 0.0f });
            heatFill->SetSizeDelta({ 0.0f, 10.0f });
            heatFill->SetBackgroundColor(Engine::Color(0.85f, 0.35f, 0.10f, 1.0f));
            m_zoneHeatBar[idx] = heatFill;

            m_zoneCCTV[idx] = card->CreateChild<UI::Text>();
            m_zoneCCTV[idx]->SetAnchors({ 0.0f, 0.0f }, { 0.0f, 0.0f });
            m_zoneCCTV[idx]->SetPivot({ 0.0f, 0.5f });
            m_zoneCCTV[idx]->SetAnchoredPosition({ 12.0f, 16.0f });
            m_zoneCCTV[idx]->SetSizeDelta({ 50.0f, 18.0f });
            m_zoneCCTV[idx]->SetFontSize(0.88f);
            m_zoneCCTV[idx]->SetColor(Engine::Color(0.9f, 0.9f, 0.9f, 1.0f));
            m_zoneCCTV[idx]->SetText("cctv:");
            m_zoneCCTV[idx]->SetAnchor(UI::TextAnchor::MiddleLeft);

            UI::Panel* cctvBg = card->CreateChild<UI::Panel>();
            cctvBg->SetAnchors({ 0.0f, 0.0f }, { 0.0f, 0.0f });
            cctvBg->SetPivot({ 0.0f, 0.5f });
            cctvBg->SetAnchoredPosition({ 78.0f, 16.0f });
            cctvBg->SetSizeDelta({ 120.0f, 10.0f });
            cctvBg->SetBackgroundColor(Engine::Color(0.25f, 0.25f, 0.25f, 1.0f));

            UI::Panel* cctvFill = cctvBg->CreateChild<UI::Panel>();
            cctvFill->SetAnchors({ 0.0f, 0.5f }, { 0.0f, 0.5f });
            cctvFill->SetPivot({ 0.0f, 0.5f });
            cctvFill->SetAnchoredPosition({ 0.0f, 0.0f });
            cctvFill->SetSizeDelta({ 0.0f, 10.0f });
            cctvFill->SetBackgroundColor(Engine::Color(0.30f, 0.55f, 0.90f, 1.0f));
            m_zoneCCTVBar[idx] = cctvFill;
            };

        MakeCard(1, { 0.03f, 0.52f }, { 0.48f, 0.90f });
        MakeCard(2, { 0.52f, 0.52f }, { 0.97f, 0.90f });
        MakeCard(3, { 0.03f, 0.08f }, { 0.48f, 0.46f });
        MakeCard(4, { 0.52f, 0.08f }, { 0.97f, 0.46f });
    }


    void BanditVisualizer::BuildCCTVPanel(UI::Panel* parent) {
        UI::Panel* cp = parent->CreateChild<UI::Panel>();
        cp->name = "CCTVPanel";
        cp->SetAnchors({ 0.70f, 0.06f }, { 0.97f, 0.86f });
        cp->SetPivot({ 0.5f, 0.5f });
        cp->SetAnchoredPosition({ 0.0f, 0.0f });
        cp->SetSizeDelta({ 0.0f, 0.0f });
        cp->SetBackgroundColor(Engine::Color(0.07f, 0.07f, 0.07f, 0.95f));

        UI::Text* title = cp->CreateChild<UI::Text>();
        title->name = "CCTVTitle";
        title->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        title->SetPivot({ 0.5f, 1.0f });
        title->SetAnchoredPosition({ 0.0f, -8.0f });
        title->SetSizeDelta({ 0.0f, 22.0f });
        title->SetFontSize(1.1f);
        title->SetColor(Engine::Color(0.85f, 0.85f, 0.85f, 1.0f));
        title->SetText("CCTV HISTORY");
        title->SetAnchor(UI::TextAnchor::UpperCenter);

        UI::Panel* sep = cp->CreateChild<UI::Panel>();
        sep->SetAnchors({ 0.03f, 1.0f }, { 0.97f, 1.0f });
        sep->SetPivot({ 0.5f, 1.0f });
        sep->SetAnchoredPosition({ 0.0f, -34.0f });
        sep->SetSizeDelta({ 0.0f, 1.0f });
        sep->SetBackgroundColor(Engine::Color(0.3f, 0.3f, 0.3f, 0.8f));

        for (int i = 0; i < kMaxCCTVEntries; ++i) {
            UI::Text* entry = cp->CreateChild<UI::Text>();
            entry->name = std::string("CCTVEntry_") + std::to_string(i);

            float top = 0.80f - i * 0.055f;

            entry->SetAnchors({ 0.04f, top - 0.045f }, { 0.88f, top });
            entry->SetPivot({ 0.0f, 1.0f });
            entry->SetAnchoredPosition({ 0.0f, 0.0f });
            entry->SetSizeDelta({ 0.0f, 0.0f });
            entry->SetFontSize(0.74f);
            entry->SetColor(Engine::Color(0.78f, 0.78f, 0.78f, 1.0f));
            entry->SetText("");
            entry->SetAnchor(UI::TextAnchor::UpperLeft);

            m_cctvEntries[i] = entry;
        }

        UI::Panel* bottom = cp->CreateChild<UI::Panel>();
        bottom->SetAnchors({ 0.0f, 0.0f }, { 0.0f, 0.0f });
        bottom->SetPivot({ 0.0f, 0.0f });
        bottom->SetAnchoredPosition({ 16.0f, 20.0f });
        bottom->SetSizeDelta({ 280.0f, 70.0f });
        bottom->SetBackgroundColor(Engine::Color(0.0f, 0.0f, 0.0f, 0.0f));

        m_cctvCurrent = bottom->CreateChild<UI::Text>();
        m_cctvCurrent->SetAnchors({ 0.0f, 1.0f }, { 0.0f, 1.0f });
        m_cctvCurrent->SetPivot({ 0.0f, 1.0f });
        m_cctvCurrent->SetAnchoredPosition({ 0.0f, 0.0f });
        m_cctvCurrent->SetSizeDelta({ 280.0f, 18.0f });
        m_cctvCurrent->SetFontSize(0.82f);
        m_cctvCurrent->SetColor(Engine::Color(0.85f, 0.85f, 0.85f, 1.0f));
        m_cctvCurrent->SetText("Current CCTV Zone: N/A");
        m_cctvCurrent->SetAnchor(UI::TextAnchor::UpperLeft);

        m_cctvCurrentPos = bottom->CreateChild<UI::Text>();
        m_cctvCurrentPos->SetAnchors({ 0.0f, 1.0f }, { 0.0f, 1.0f });
        m_cctvCurrentPos->SetPivot({ 0.0f, 1.0f });
        m_cctvCurrentPos->SetAnchoredPosition({ 0.0f, -22.0f });
        m_cctvCurrentPos->SetSizeDelta({ 280.0f, 18.0f });
        m_cctvCurrentPos->SetFontSize(0.82f);
        m_cctvCurrentPos->SetColor(Engine::Color(0.85f, 0.85f, 0.85f, 1.0f));
        m_cctvCurrentPos->SetText("Current Position: (0, 0)");
        m_cctvCurrentPos->SetAnchor(UI::TextAnchor::UpperLeft);

        m_cctvCurrentStatus = bottom->CreateChild<UI::Text>();
        m_cctvCurrentStatus->SetAnchors({ 0.0f, 1.0f }, { 0.0f, 1.0f });
        m_cctvCurrentStatus->SetPivot({ 0.0f, 1.0f });
        m_cctvCurrentStatus->SetAnchoredPosition({ 0.0f, -44.0f });
        m_cctvCurrentStatus->SetSizeDelta({ 280.0f, 18.0f });
        m_cctvCurrentStatus->SetFontSize(0.82f);
        m_cctvCurrentStatus->SetColor(Engine::Color(0.50f, 1.0f, 0.50f, 1.0f));
        m_cctvCurrentStatus->SetText("Status: Lost");
        m_cctvCurrentStatus->SetAnchor(UI::TextAnchor::UpperLeft);
    }

    void BanditVisualizer::Update() {
        const auto& cfg = GetBanditVisualizerConfig();
        float dt = Time::deltaTime;
        if (dt <= 0.0f) dt = cfg.dtFallback;

        m_elapsedTime += dt;
        m_timer += dt;

        if (m_cctvLogCooldown > 0.0f) {
            m_cctvLogCooldown -= dt;
            if (m_cctvLogCooldown < 0.0f) m_cctvLogCooldown = 0.0f;
        }

        if (m_timer < updateInterval) return;
        m_timer = 0.0f;

        if (!bandit) {
            RefreshUI();
            return;
        }

        int pulls[4];
        float means[4];
        for (int i = 0; i < 4; ++i) {
            pulls[i] = bandit->Pulls(i);
            means[i] = bandit->Value(i);
        }

        float cctv[5] = {};
        if (cctvScores) {
            for (int z = 1; z <= 4; ++z) cctv[z] = cctvScores[z];
        }

        float heatZones[5] = {};
        if (m_heatmap && m_zoneGrid) {
            for (int z = 1; z <= 4; ++z) {
                heatZones[z] = m_heatmap->SumZone(*m_zoneGrid, z);
            }
        }

        int selectedZone = m_selectedZone;
        if (selectedZone < 1 || selectedZone > 4) selectedZone = 1;

        ZoneBreakdown bd{};
        int idx = selectedZone - 1;
        bd.exploitation = means[idx];
        bd.exploration = (pulls[idx] > 0) ? 0.0f : 2.13f;
        bd.heatInfluence = heatZones[selectedZone];
        bd.cctvInfluence = cctv[selectedZone];
        bd.finalScore = bd.exploitation + bd.exploration + bd.heatInfluence + bd.cctvInfluence;

        std::string reason;
        if (bd.heatInfluence >= bd.exploitation && bd.heatInfluence >= bd.cctvInfluence && bd.heatInfluence > 0.0f) {
            reason = ZoneLetter(selectedZone) + " chosen -> high heat";
        }
        else if (bd.exploration > 0.0f) {
            reason = ZoneLetter(selectedZone) + " chosen -> exploration";
        }
        else if (bd.cctvInfluence > 0.0f) {
            reason = ZoneLetter(selectedZone) + " chosen -> CCTV activity";
        }
        else {
            reason = ZoneLetter(selectedZone) + " chosen -> highest current score";
        }

        SetMLSummary(selectedZone, 0.0f, false, pulls, means, cctv, heatZones, &bd, reason.c_str());
    }

    void BanditVisualizer::ToggleVisibility() {
        if (!m_panel) return;
        SetVisible(!m_panel->visible);
    }

    void BanditVisualizer::SetVisible(bool v) {
        if (!m_panel) return;
        m_panel->visible = v;
    }

    void BanditVisualizer::SetSelectedZone(int zid)
    {
        if (zid < 1 || zid > 4) return;

        m_selectedZone = zid;

        if (m_selectedZoneText)
            m_selectedZoneText->SetText("Selected Zone: " + std::to_string(zid) + " *");

        RefreshZoneCards();
        RefreshBreakdown();
    }

    void BanditVisualizer::SetMLSummary(int priorityZone, float hotTimer, bool fullEscalation,
        const int pulls[4],
        const float means[4],
        const float cctv[5],
        const float heatZones[5],
        const ZoneBreakdown* breakdown,
        const char* reason)
    {
        const auto& cfg = GetBanditVisualizerConfig();
        m_selectedZone = priorityZone;

        if (m_selectedZoneText)
            m_selectedZoneText->SetText(("Selected Zone: " + std::to_string(priorityZone) + " *").c_str());

        if (m_hotTimerText)
            m_hotTimerText->SetText(("Hot Timer: " + FmtFloat(hotTimer)).c_str());

        if (m_escalationText)
            m_escalationText->SetText((std::string("Escalation: ") + (fullEscalation ? "ON" : "OFF")).c_str());

        if (m_reasonText) {
            std::string r = (reason && strlen(reason) > 0) ? reason : "UCB default decision";
            m_reasonText->SetText(("Reason: " + r).c_str());
        }

        if (breakdown)
            m_lastBreakdown = *breakdown;

        // use fixed refs so bar reflects actual zone heat sum / cctv score
        const float maxHeat = cfg.maxHeat;
        const float maxCCTV = cfg.maxCCTV;

        for (int i = 0; i < 4; ++i) {
            int z = i + 1;

            if (m_zonePulls[i])
                m_zonePulls[i]->SetText(("pulls: " + std::to_string(pulls[i])).c_str());

            if (m_zoneMeans[i])
                m_zoneMeans[i]->SetText(("mean: " + FmtFloat(means[i], 2)).c_str());

            if (m_zoneHeat[i])
                m_zoneHeat[i]->SetText(("heat: " + FmtFloat(heatZones[z], 1)).c_str());

            if (m_zoneCCTV[i])
                m_zoneCCTV[i]->SetText(("cctv: " + FmtFloat(cctv[z], 1)).c_str());

            if (m_zoneHeatBar[i])
                UpdateBarFill(m_zoneHeatBar[i], heatZones[z], maxHeat);

            if (m_zoneCCTVBar[i])
                UpdateBarFill(m_zoneCCTVBar[i], cctv[z], maxCCTV);

            if (m_zoneCards[i]) {
                bool selected = (z == m_selectedZone);
                m_zoneCards[i]->SetBackgroundColor(
                    selected
                    ? Engine::Color(0.18f, 0.18f, 0.07f, 0.98f)
                    : Engine::Color(0.09f, 0.09f, 0.09f, 0.95f)
                );
            }
        }

        RefreshBreakdown();
        RefreshCCTVLog();
    }

    void BanditVisualizer::PushCCTVRecord(const CCTVRecord& rec) {
        m_cctvHistory.push_front(rec);
        while ((int)m_cctvHistory.size() > kMaxCCTVEntries)
            m_cctvHistory.pop_back();
        RefreshCCTVLog();
    }

    void BanditVisualizer::SetCCTVStatus(int zone, float posX, float posY, bool visible)
    {
        m_elapsedTime += Time::deltaTime;

        bool shouldLog = false;
        CCTVRecord rec{};

      
        if (!m_hasCCTVState) {
            m_hasCCTVState = true;

            if (visible) {
                shouldLog = true;
                rec = { m_elapsedTime, posX, posY, zone, false, false };
            }
        }
        else {
        
            if (!m_lastCCTVVisible && visible) {
                shouldLog = true;
                rec = { m_elapsedTime, posX, posY, zone, false, true };
            }
            
            else if (m_lastCCTVVisible && !visible) {
                shouldLog = true;
                rec = { m_elapsedTime, posX, posY, m_lastCCTVZone, true, false };
            }
        }

      
        if (shouldLog) {
            m_cctvHistory.push_front(rec);

            if ((int)m_cctvHistory.size() > kMaxCCTVEntries)
                m_cctvHistory.pop_back();
        }

        m_lastCCTVVisible = visible;
        m_lastCCTVZone = zone;
        m_lastCCTVPosX = posX;
        m_lastCCTVPosY = posY;

        RefreshCCTVLog();
    }


    void BanditVisualizer::RefreshUI() {
        RefreshZoneCards();
        RefreshBreakdown();
        RefreshCCTVLog();
    }

    void BanditVisualizer::RefreshZoneCards() {
        if (!bandit) {
            for (int i = 0; i < 4; ++i) {
                if (m_zonePulls[i]) m_zonePulls[i]->SetText("pulls: --");
                if (m_zoneMeans[i]) m_zoneMeans[i]->SetText("mean: --");
                if (m_zoneHeatBar[i]) UpdateBarFill(m_zoneHeatBar[i], 0.0f, 1.0f);
                if (m_zoneCCTVBar[i]) UpdateBarFill(m_zoneCCTVBar[i], 0.0f, 1.0f);
            }
            return;
        }

        for (int i = 0; i < 4; ++i) {
            if (m_zonePulls[i])
                m_zonePulls[i]->SetText(("pulls: " + std::to_string(bandit->Pulls(i))).c_str());
            if (m_zoneMeans[i])
                m_zoneMeans[i]->SetText(("mean: " + FmtFloat(bandit->Value(i), 2)).c_str());
        }
    }

    void BanditVisualizer::RefreshBreakdown() {
        if (!m_breakdownZoneLabel) return;

        if (m_selectedZone < 1 || m_selectedZone > 4) {
            m_breakdownZoneLabel->SetText("Zone: N/A");
            for (int i = 0; i < 5; ++i) {
                if (m_detailLines[i]) m_detailLines[i]->SetText("---");
            }
            return;
        }

        m_breakdownZoneLabel->SetText((std::string("Zone: ") + std::to_string(m_selectedZone) + " *").c_str());

        const char* labels[] = {
            "Exploitation (mean): ",
            "Exploration bonus:   ",
            "Heat influence:      ",
            "CCTV influence:      ",
            "Final score:         "
        };

        const float vals[] = {
            m_lastBreakdown.exploitation,
            m_lastBreakdown.exploration,
            m_lastBreakdown.heatInfluence,
            m_lastBreakdown.cctvInfluence,
            m_lastBreakdown.finalScore
        };

        for (int i = 0; i < 5; ++i) {
            if (!m_detailLines[i]) continue;
            std::string s = std::string(labels[i]) + FmtFloat(vals[i], 2);
            if (i == 4) s += "  *";
            m_detailLines[i]->SetText(s.c_str());
        }
    }

    void BanditVisualizer::RefreshCCTVLog()
    {
        for (int i = 0; i < kMaxCCTVEntries; ++i) {
            if (!m_cctvEntries[i]) continue;

            if (i >= (int)m_cctvHistory.size()) {
                m_cctvEntries[i]->SetText("");
                continue;
            }

            const CCTVRecord& r = m_cctvHistory[i];
            char buf[96];

            if (r.lostSight) {
                snprintf(buf, sizeof(buf), "[%s] %s lost player",
                    FmtFloat(r.timestamp, 2).c_str(),
                    ZoneLetter(r.zone).c_str());
            }
            else if (r.reDetected) {
                snprintf(buf, sizeof(buf), "[%s] %s re-detected",
                    FmtFloat(r.timestamp, 2).c_str(),
                    ZoneLetter(r.zone).c_str());
            }
            else {
                snprintf(buf, sizeof(buf), "[%s] %s cam (%.1f, %.1f)",
                    FmtFloat(r.timestamp, 2).c_str(),
                    ZoneLetter(r.zone).c_str(),
                    r.posX, r.posY);
            }

            m_cctvEntries[i]->SetText(buf);
        }

        if (m_cctvCurrent)
            m_cctvCurrent->SetText("Current CCTV Zone: " + ((m_lastCCTVZone > 0) ? ZoneLetter(m_lastCCTVZone) : "N/A"));

        if (m_cctvCurrentPos)
            m_cctvCurrentPos->SetText("Current Position: (" + FmtFloat(m_lastCCTVPosX, 1) + ", " + FmtFloat(m_lastCCTVPosY, 1) + ")");

        if (m_cctvCurrentStatus)
            m_cctvCurrentStatus->SetText(std::string("Status: ") + (m_lastCCTVVisible ? "Visible" : "Lost"));
    }

    void BanditVisualizer::UpdateBarFill(UI::Panel* bar, float value, float maxValue)
    {
        if (!bar) return;

        float t = 0.0f;
        if (maxValue > 0.0001f)
            t = value / maxValue;

        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        const float kBarWidth = GetBanditVisualizerConfig().barWidth;

        bar->SetAnchors({ 0.0f, 0.5f }, { 0.0f, 0.5f });
        bar->SetPivot({ 0.0f, 0.5f });
        bar->SetAnchoredPosition({ 0.0f, 0.0f });
        bar->SetSizeDelta({ kBarWidth * t, 10.0f });
    }
    void BanditVisualizer::OnDestroy() {
        if (m_canvas && UI::UIManager::GetInstance()) {
            UI::UIManager::GetInstance()->DestroyCanvas(m_canvas);
            m_canvas = nullptr;
        }

        m_panel = nullptr;

        m_headerTitle = nullptr;
        m_selectedZoneText = nullptr;
        m_escalationText = nullptr;
        m_hotTimerText = nullptr;
        m_reasonText = nullptr;

        for (int i = 0; i < 4; ++i) {
            m_zoneCards[i] = nullptr;
            m_zonePulls[i] = nullptr;
            m_zoneMeans[i] = nullptr;
            m_zoneHeat[i] = nullptr;
            m_zoneHeatBar[i] = nullptr;
            m_zoneCCTV[i] = nullptr;
            m_zoneCCTVBar[i] = nullptr;
            m_heatTexts[i] = nullptr;
        }

        m_breakdownZoneLabel = nullptr;
        for (int i = 0; i < 5; ++i) {
            m_detailLines[i] = nullptr;
        }

        m_cctvCurrent = nullptr;
        m_cctvCurrentPos = nullptr;
        m_cctvCurrentStatus = nullptr;
        for (int i = 0; i < 10; ++i) {
            m_cctvEntries[i] = nullptr;
        }

        for (int i = 0; i < 9; ++i) {
            m_texts[i] = nullptr;
        }

        m_summary = nullptr;
        m_priorityText = nullptr;
        m_cctvText = nullptr;
    }

} // namespace Game