#include "BanditVisualizer.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/Core/Time.hpp"
#include <sstream>
#include <iomanip>
#include <string>

namespace Game {

    static const char* ZoneIdToLetter(int id) {
        switch (id) {
        case 1: return "1";
        case 2: return "2";
        case 3: return "3";
        case 4: return "4";
        default: return "N/A";
        }
    }

    BanditVisualizer::BanditVisualizer() {}
    BanditVisualizer::~BanditVisualizer() {}

    void BanditVisualizer::Initialize() {
        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        if (!m_canvas) return;

        m_canvas->SetReferenceResolution(1280.0f, 720.0f);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);
        m_canvas->SetSortOrder(1000);

        m_panel = m_canvas->CreateChild<UI::Panel>();
        m_panel->name = "BanditDebugPanel";
        m_panel->rectTransform.SetCentered(620.0f, 360.0f);
        m_panel->SetPivot({ 0.5f, 0.5f });
        m_panel->SetBackgroundColor(Engine::Color(0.05f, 0.05f, 0.05f, 0.90f));
        m_panel->SetSizeDelta({ 620.0f, 360.0f });
        m_panel->visible = false;

        m_priorityText = m_panel->CreateChild<UI::Text>();
        m_priorityText->name = "BanditPriority";
        m_priorityText->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
        m_priorityText->SetAnchoredPosition({ 0.0f, -20.0f });
        m_priorityText->SetSizeDelta({ -40.0f, 30.0f });
        m_priorityText->SetPivot({ 0.5f, 1.0f });
        m_priorityText->SetFontSize(1.2f);
        m_priorityText->SetColor(Engine::Color::white());
        m_priorityText->SetText("Selected Zone: N/A");
        m_priorityText->SetAnchor(UI::TextAnchor::MiddleCenter);
        m_priorityText->visible = false;

        for (int i = 0; i < 9; ++i) {
            m_texts[i] = m_panel->CreateChild<UI::Text>();
            m_texts[i]->name = std::string("BanditText_") + std::to_string(i);

            float y = -55.0f - i * 22.0f;

            m_texts[i]->SetAnchors({ 0.0f, 1.0f }, { 1.0f, 1.0f });
            m_texts[i]->SetAnchoredPosition({ 0.0f, y });
            m_texts[i]->SetSizeDelta({ -40.0f, 22.0f });
            m_texts[i]->SetPivot({ 0.5f, 1.0f });
            m_texts[i]->SetFontSize(1.0f);
            m_texts[i]->SetColor(Engine::Color::white());
            m_texts[i]->SetText("---");
            m_texts[i]->SetAnchor(UI::TextAnchor::MiddleLeft);
            m_texts[i]->visible = false;
        }

        m_summary = nullptr;
        m_cctvText = nullptr;

        for (int i = 0; i < 4; ++i) {
            m_heatTexts[i] = nullptr;
        }

        m_selectedZone = -1;
        m_timer = 0.0f;

        RefreshUI();
    }

    void BanditVisualizer::Update() {
        float dt = Time::deltaTime;
        if (dt <= 0.0f) dt = 0.016f;

        m_timer += dt;
        if (m_timer >= updateInterval) {
            m_timer = 0.0f;
            RefreshUI();

            if (bandit) {
                int pulls[4] = {
                    bandit->Pulls(0),
                    bandit->Pulls(1),
                    bandit->Pulls(2),
                    bandit->Pulls(3)
                };

                float means[4] = {
                    bandit->Value(0),
                    bandit->Value(1),
                    bandit->Value(2),
                    bandit->Value(3)
                };

                float cctv[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
                if (cctvScores) {
                    cctv[1] = cctvScores[1];
                    cctv[2] = cctvScores[2];
                    cctv[3] = cctvScores[3];
                    cctv[4] = cctvScores[4];
                }

                float heatZones[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

                // If we have a heatmap and the scene's zone grid, compute per-zone heat sums.
                if (m_heatmap && m_zoneGrid) {
                    // Heatmap::SumZone expects the full zoneGrid vector and a zone id (1..4)
                    for (int z = 1; z <= 4; ++z) {
                        heatZones[z] = m_heatmap->SumZone(*m_zoneGrid, z);
                    }
                }

                SetMLSummary(m_selectedZone, 0.0f, false, pulls, means, cctv, heatZones);
            }
            else {
                for (int i = 4; i < 9; ++i) {
                    if (m_texts[i]) m_texts[i]->SetText("---");
                }
            }
        }
    }

    void BanditVisualizer::OnDestroy() {
        if (m_canvas && UI::UIManager::GetInstance()) {
            UI::UIManager::GetInstance()->DestroyCanvas(m_canvas);
            m_canvas = nullptr;
        }

        m_panel = nullptr;
        m_priorityText = nullptr;
        m_summary = nullptr;
        m_cctvText = nullptr;

        for (int i = 0; i < 9; ++i) {
            m_texts[i] = nullptr;
        }

        for (int i = 0; i < 4; ++i) {
            m_heatTexts[i] = nullptr;
        }
    }

    void BanditVisualizer::ToggleVisibility() {
        if (!m_panel) return;

        m_panel->visible = !m_panel->visible;
        bool v = m_panel->visible;

        if (m_priorityText) m_priorityText->visible = v;

        for (int i = 0; i < 9; ++i) {
            if (m_texts[i]) m_texts[i]->visible = v;
        }
    }

    void BanditVisualizer::SetVisible(bool v) {
        if (!m_panel) return;

        m_panel->visible = v;

        if (m_priorityText) m_priorityText->visible = v;

        for (int i = 0; i < 9; ++i) {
            if (m_texts[i]) m_texts[i]->visible = v;
        }
    }

    void BanditVisualizer::SetSelectedZone(int zid) {
        if (zid >= 0 && zid <= 3) zid = zid + 1;
        if (zid == m_selectedZone) return;

        m_selectedZone = zid;

        if (m_priorityText) {
            if (m_selectedZone > 0) {
                m_priorityText->SetText(std::string("Selected Zone: ") + ZoneIdToLetter(m_selectedZone));
            }
            else {
                m_priorityText->SetText("Selected Zone: N/A");
            }
        }

        RefreshUI();
    }

    void BanditVisualizer::RefreshUI() {
        if (!bandit) {
            for (int i = 0; i < 4; ++i) {
                if (m_texts[i]) {
                    std::string s = std::string("Zone ") + ZoneIdToLetter(i + 1) + ": N/A";
                    m_texts[i]->SetText(s);
                }
            }

            for (int i = 4; i < 9; ++i) {
                if (m_texts[i]) {
                    m_texts[i]->SetText("---");
                }
            }
            return;
        }

        for (int i = 0; i < 4; ++i) {
            int pulls = bandit->Pulls(i);
            float value = bandit->Value(i);

            std::ostringstream line;
            line << "Zone " << ZoneIdToLetter(i + 1);
            if (m_selectedZone == (i + 1)) line << " [PRIOR]";
            line << "  pulls=" << pulls
                << "  mean=" << std::fixed << std::setprecision(3) << value;

            if (m_texts[i]) m_texts[i]->SetText(line.str());
        }
    }

    void BanditVisualizer::SetMLSummary(int priorityZone, float hotTimer, bool fullEscalation,
        const int pulls[4], const float means[4],
        const float cctv[5], const float heatZones[5]) {

        if (!m_texts[4]) return;

        m_texts[4]->SetText(
            std::string("Priority: ") + ZoneIdToLetter(priorityZone) +
            " | HotTimer: " + std::to_string(hotTimer)
        );

        m_texts[5]->SetText(
            std::string("Escalation: ") + (fullEscalation ? "YES" : "NO") +
            " | Pulls: [" +
            std::to_string(pulls[0]) + "," +
            std::to_string(pulls[1]) + "," +
            std::to_string(pulls[2]) + "," +
            std::to_string(pulls[3]) + "]"
        );

        m_texts[6]->SetText(
            std::string("Means: [") +
            std::to_string(means[0]) + "," +
            std::to_string(means[1]) + "," +
            std::to_string(means[2]) + "," +
            std::to_string(means[3]) + "]"
        );

  
        m_texts[7]->SetText(
            std::string("CCTV: [") +
            std::to_string(cctv[1]) + "," +
            std::to_string(cctv[2]) + "," +
            std::to_string(cctv[3]) + "," +
            std::to_string(cctv[4]) + "]"
        );

     
        m_texts[8]->SetText(
            std::string("Heat: [") +
            std::to_string(heatZones[1]) + "," +
            std::to_string(heatZones[2]) + "," +
            std::to_string(heatZones[3]) + "," +
            std::to_string(heatZones[4]) + "]"
        );
    }

} // namespace Game