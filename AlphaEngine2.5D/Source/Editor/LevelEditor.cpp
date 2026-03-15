/**
 * @file Editor/LevelEditor.cpp
 * @brief Optimized ImGui LevelEditor implementation
 */
#include "LevelEditor.hpp"
#include <fstream>
#include <algorithm>
#include <string>

#include "Engine/Scene/Scene.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include "Engine/TileMap/Door.hpp"
#include "Engine/Graphics/DoorRenderer.hpp"
#include "Engine/Physics/BoxCollider2D.hpp"
#include "Engine/Interaction/Interactable.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Graphics/TileMapRenderer.hpp"
#include "Engine/Core/AssetManager.hpp"
#include "Game/CCTV/CCTVTerminal.hpp"
#include "Engine/Scene/SceneManager.hpp"

namespace AEEditor
{
    Engine::Scene *LevelEditor::s_scene = nullptr;
    Engine::TileMap *LevelEditor::s_tileMap = nullptr;

    int LevelEditor::s_currentTileValue = 1;
    bool LevelEditor::s_is3DMode = false;

    int LevelEditor::s_mapWidth = 0;
    int LevelEditor::s_mapHeight = 0;
    float LevelEditor::s_tileSize = 32.0f;

    ImVec2 LevelEditor::s_scrolling = ImVec2(0.0f, 0.0f);
    float LevelEditor::s_zoom = 1.0f;
    int LevelEditor::s_hoveredX = -1;
    int LevelEditor::s_hoveredY = -1;

    bool LevelEditor::Is3DMode() { return s_is3DMode; }

    void LevelEditor::Initialize(Engine::Scene *scene, Engine::TileMap *tileMap)
    {
        s_scene = scene;
        s_tileMap = tileMap;

        if (s_tileMap)
        {
            s_mapWidth = s_tileMap->width;
            s_mapHeight = s_tileMap->height;
        }
    }

    void LevelEditor::Exit() {}

    void LevelEditor::DrawAssetBrowser()
    {
        ImGui::Begin("Toolbar");

        if (s_hoveredX >= 0)
            ImGui::Text("Hovered Tile: (%d, %d)", s_hoveredX, s_hoveredY);
        else
            ImGui::Text("Hovered Tile: (--, --)");

        ImGui::Separator();

        ImGui::Text("Map Editing Modes");
        ImGui::Separator();

        ImGui::BeginDisabled(s_is3DMode);

        struct ModeDef
        {
            const char *name;
            int value;
        };
        ModeDef modes[] = {
            {"Wall (1)", 1}, {"Door (2)", 10}, {"Terminal (3)", 11}, {"Exit (4)", 12}, {"Ctrl Room (5)", 20}, {"Green Card (6)", 15}, {"Purple Card (7)", 16}, {"Red Card (8)", 17}, {"Yellow Card (9)", 18}, {"Erase (0)", 0}};

        for (int i = 0; i < 10; i++)
        {
            bool isSelected = (s_currentTileValue == modes[i].value);
            if (isSelected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));

            if (ImGui::Button(modes[i].name, ImVec2(120, 30)))
            {
                s_currentTileValue = modes[i].value;
            }

            if (isSelected)
                ImGui::PopStyleColor();

            if (i % 2 == 0)
                ImGui::SameLine();
        }

        ImGui::EndDisabled();

        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Button, s_is3DMode ? ImVec4(0.2f, 0.5f, 0.8f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        if (ImGui::Button(s_is3DMode ? "Viewport: 3D Scene" : "Viewport: 2D Map", ImVec2(248, 40)))
        {
            s_is3DMode = !s_is3DMode;

            if (s_is3DMode)
            {
                SaveMapTxt("Assets/map_tmp.txt");
                Engine::SceneManager::LoadScene("EditorScene");
            }
        }
        ImGui::PopStyleColor();

        if (ImGui::Button("Save Map (S)", ImVec2(248, 40)))
        {
            SaveMapTxt("Assets/map.txt");
        }
        ImGui::End();
    }

    ImU32 LevelEditor::GetTileColor(int tileValue)
    {
        switch (tileValue)
        {
        case 0:
            return IM_COL32(90, 90, 90, 255);
        case 1:
            return IM_COL32(220, 220, 220, 255);
        case 10:
            return IM_COL32(100, 200, 255, 255);
        case 11:
            return IM_COL32(255, 200, 100, 255);
        case 12:
            return IM_COL32(100, 255, 100, 255);
        case 13:
            return IM_COL32(255, 100, 255, 255);
        case 20:
            return IM_COL32(255, 50, 255, 255);
        case 15:
            return IM_COL32(50, 255, 50, 255);
        case 16:
            return IM_COL32(200, 50, 255, 255);
        case 17:
            return IM_COL32(255, 50, 50, 255);
        case 18:
            return IM_COL32(255, 255, 50, 255);
        default:
            return IM_COL32(255, 100, 255, 255);
        }
    }

    void LevelEditor::DrawMapEditorViewport()
    {
        if (s_is3DMode)
        {
            ImGui::TextDisabled("3D Viewport Active (Engine Render Pass)");
            return;
        }

        if (!s_tileMap)
            return;

        ImGuiIO &io = ImGui::GetIO();
        ImDrawList *drawList = ImGui::GetWindowDrawList();

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();

        ImVec2 canvasMax = ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);
        drawList->AddRectFilled(canvasPos, canvasMax, IM_COL32(40, 40, 40, 255));
        if (!s_tileMap)
        {
            drawList->AddText(ImVec2(canvasPos.x + 10, canvasPos.y + 10), IM_COL32(255, 0, 0, 255), "no map");
            return;
        }
        if (ImGui::IsWindowHovered())
        {
            if (io.MouseWheel != 0.0f)
            {
                float zoomDelta = io.MouseWheel * 0.1f;
                s_zoom = std::clamp(s_zoom + zoomDelta, 0.1f, 5.0f);
            }

            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
            {
                s_scrolling.x += io.MouseDelta.x;
                s_scrolling.y += io.MouseDelta.y;
            }
        }

        drawList->AddRectFilled(canvasPos, canvasMax, IM_COL32(40, 40, 40, 255));

        float scaledTileSize = s_tileSize * s_zoom;

        ImVec2 origin(canvasPos.x + s_scrolling.x,
                      canvasPos.y + s_scrolling.y);

        int firstX = max(0, (int)std::floor((canvasPos.x - origin.x) / scaledTileSize));
        int firstY = max(0, (int)std::floor((canvasPos.y - origin.y) / scaledTileSize));
        int lastX = min(s_mapWidth - 1, (int)std::ceil((canvasMax.x - origin.x) / scaledTileSize));
        int lastY = min(s_mapHeight - 1, (int)std::ceil((canvasMax.y - origin.y) / scaledTileSize));

        float startX = origin.x + firstX * scaledTileSize;
        float startY = origin.y + firstY * scaledTileSize;

        for (int y = firstY; y <= lastY; ++y)
        {
            float py = startY + (y - firstY) * scaledTileSize;

            for (int x = firstX; x <= lastX; ++x)
            {
                float px = startX + (x - firstX) * scaledTileSize;

                int tileVal = (int)s_tileMap->GetTile(x, s_mapHeight - 1 - y);
                ImU32 color = GetTileColor(tileVal);

                ImVec2 pMin(px, py);
                ImVec2 pMax(px + scaledTileSize, py + scaledTileSize);

                drawList->AddRectFilled(pMin, pMax, color);
                drawList->AddRect(pMin, pMax, IM_COL32(60, 60, 60, 255));
            }
        }

        if (ImGui::IsWindowHovered())
        {
            ImVec2 mousePos = io.MousePos;
            int screenGridX = static_cast<int>(std::floor((mousePos.x - origin.x) / scaledTileSize));
            int screenGridY = static_cast<int>(std::floor((mousePos.y - origin.y) / scaledTileSize));

            if (screenGridX >= 0 && screenGridX < s_mapWidth && screenGridY >= 0 && screenGridY < s_mapHeight)
            {
                s_hoveredX = screenGridX;
                s_hoveredY = s_mapHeight - 1 - screenGridY;

                ImVec2 hMin = ImVec2(origin.x + (screenGridX * scaledTileSize), origin.y + (screenGridY * scaledTileSize));
                ImVec2 hMax = ImVec2(hMin.x + scaledTileSize, hMin.y + scaledTileSize);
                drawList->AddRect(hMin, hMax, IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);

                if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    ApplyPaint(s_hoveredX, s_hoveredY);
                }
            }
            else
            {
                s_hoveredX = -1;
                s_hoveredY = -1;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_S))
                SaveMapTxt("Assets/map.txt");
        }
    }

    void LevelEditor::ApplyPaint(int gridX, int gridY)
    {
        if (gridX < 0 || gridX >= s_mapWidth || gridY < 0 || gridY >= s_mapHeight)
            return;

        int currentTile = static_cast<int>(s_tileMap->GetTile(gridX, gridY));

        if (s_currentTileValue == 0)
        {
            s_tileMap->SetTile(gridX, gridY, static_cast<Engine::TileType>(0));
            return;
        }

        if (s_currentTileValue == 1)
        {
            s_tileMap->SetTile(gridX, gridY, static_cast<Engine::TileType>(1));
            return;
        }

        if (currentTile != 0)
            return;

        Engine::TileType newTile = static_cast<Engine::TileType>(s_currentTileValue);

        s_tileMap->SetTile(gridX, gridY, newTile);
    }

    void LevelEditor::SaveMapTxt(const char *filepath)
    {
        if (!s_tileMap)
            return;

        std::ofstream file(filepath);
        if (!file.is_open())
            return;

        for (int y = 0; y < s_mapHeight; ++y)
        {
            for (int x = 0; x < s_mapWidth; ++x)
            {
                int tile = static_cast<int>(s_tileMap->GetTile(x, y));
                char c = '0';

                switch (tile)
                {
                case 0:
                    c = '0';
                    break;
                case 1:
                    c = '1';
                    break;
                case 10:
                    c = 'D';
                    break;
                case 11:
                    c = 'T';
                    break;
                case 12:
                    c = 'E';
                    break;
                case 13:
                    c = 'C';
                    break;
                case 20:
                    c = 'K';
                    break;
                case 15:
                    c = 'F';
                    break;
                case 16:
                    c = 'G';
                    break;
                case 17:
                    c = 'H';
                    break;
                case 18:
                    c = 'J';
                    break;
                default:
                    c = '0';
                    break;
                }
                file << c;
            }
            file << '\n';
        }
        file.close();
    }
}