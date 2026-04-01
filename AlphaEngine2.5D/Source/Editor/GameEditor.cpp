/* Start Header ************************************************************************/
/*!
\file       GameEditor.cpp
\author
\par
\date   March, 29, 2026
\brief      [AUTO] GameEditor

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "GameEditor.hpp"
#include "ObjectManager.hpp"
#include <fstream>
#include <algorithm>
#include <string>
#include <filesystem>

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
    Engine::Scene *GameEditor::scene = nullptr;
    Engine::TileMap *GameEditor::tilemap = nullptr;

    int GameEditor::currentTileValue = 1;
    bool GameEditor::is3DMode = false;

    int GameEditor::mapWidth = 0;
    int GameEditor::mapHeight = 0;
    float GameEditor::tileSize = 32.0f;

    ImVec2 GameEditor::scrolling = ImVec2(0.0f, 0.0f);
    float GameEditor::zoom = 1.0f;
    int GameEditor::hoveredX = -1;
    int GameEditor::hoveredY = -1;

    /*!
     * \brief [AUTO] Is3DMode
     * \return
     */
    bool GameEditor::Is3DMode() { return is3DMode; }

    void GameEditor::Initialize(Engine::Scene *s, Engine::TileMap *tileMap)
    {
        scene = s;
        tilemap = tileMap;

        if (tilemap)
        {
            mapWidth = tilemap->width;
            mapHeight = tilemap->height;
        }

        ObjectManager::LoadSceneFromJson("Assets/Config/Scenes/EditorScene.json");
    }

    void GameEditor::Exit()
    {
        std::filesystem::remove("Assets/tmpMap.txt");
    }

    void GameEditor::DrawAssetBrowser()
    {
        ImGui::Begin("Toolbar");

        if (hoveredX >= 0)
            ImGui::Text("Hovered Tile: (%d, %d)", hoveredX, hoveredY);
        else
            ImGui::Text("Hovered Tile: (--, --)");

        ImGui::Separator();

        ImGui::Text("Map Editing Modes");
        ImGui::Separator();

        ImGui::BeginDisabled(is3DMode);

        struct ModeDef
        {
            const char *name;
            int value;
        };
        ModeDef modes[] = {
            {"Wall (1)", 1}, {"Erase (0)", 0}};

        for (int i = 0; i < 2; i++)
        {
            bool isSelected = (currentTileValue == modes[i].value);
            if (isSelected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));

            if (ImGui::Button(modes[i].name, ImVec2(120, 30)))
            {
                currentTileValue = modes[i].value;
                ObjectManager::selectedObject = nullptr;
            }

            if (isSelected)
                ImGui::PopStyleColor();

            if (i % 2 == 0)
                ImGui::SameLine();
        }

        ImGui::EndDisabled();

        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Button, is3DMode ? ImVec4(0.2f, 0.5f, 0.8f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        if (ImGui::Button(is3DMode ? "Viewport: 3D Scene" : "Viewport: 2D Map", ImVec2(248, 40)))
        {
            is3DMode = !is3DMode;

            if (is3DMode)
            {
                SaveMap("Assets/tmpMap.txt");
                ObjectManager::SaveSceneToJson("Assets/Config/Scenes/EditorScene.json");
                ObjectManager::selectedObject = nullptr;
                ObjectManager::draggingObject = nullptr;

                Engine::SceneManager::LoadScene("EditorScene");
            }
            else
            {
                ObjectManager::LoadSceneFromJson("Assets/Config/Scenes/EditorScene.json");
            }
        }
        ImGui::PopStyleColor();

        if (ImGui::Button("Save Map (S)", ImVec2(248, 40)))
        {
            SaveMap("Assets/map.txt");
            SaveMap("Assets/tmpMap.txt");
            ObjectManager::SaveSceneToJson("Assets/Config/Scenes/EditorScene.json");
        }
        ImGui::End();
    }

    ImU32 GameEditor::GetTileColor(int tileValue)
    {
        switch (tileValue)
        {
        case 0:
            return IM_COL32(90, 90, 90, 255);
        case 1:
            return IM_COL32(220, 220, 220, 255);
        default:
            return IM_COL32(255, 100, 255, 255);
        }
    }

    void GameEditor::DrawMapEditorViewport()
    {
        if (is3DMode)
        {
            ImGui::TextDisabled("3D Viewport Active (Engine Render Pass)");
            return;
        }

        if (!tilemap)
            return;

        ImGuiIO &io = ImGui::GetIO();
        ImDrawList *drawList = ImGui::GetWindowDrawList();

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();

        ImVec2 canvasMax = ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);
        drawList->AddRectFilled(canvasPos, canvasMax, IM_COL32(40, 40, 40, 255));
        if (!tilemap)
        {
            drawList->AddText(ImVec2(canvasPos.x + 10, canvasPos.y + 10), IM_COL32(255, 0, 0, 255), "no map");
            return;
        }
        if (ImGui::IsWindowHovered())
        {
            if (io.MouseWheel != 0.0f)
            {
                float zoomDelta = io.MouseWheel * 0.1f;
                zoom = std::clamp(zoom + zoomDelta, 0.1f, 5.0f);
            }

            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
            {
                scrolling.x += io.MouseDelta.x;
                scrolling.y += io.MouseDelta.y;
            }
        }

        drawList->AddRectFilled(canvasPos, canvasMax, IM_COL32(40, 40, 40, 255));

        float scaledTileSize = tileSize * zoom;

        ImVec2 origin(canvasPos.x + scrolling.x,
                      canvasPos.y + scrolling.y);

        int firstX = max(0, (int)std::floor((canvasPos.x - origin.x) / scaledTileSize));
        int firstY = max(0, (int)std::floor((canvasPos.y - origin.y) / scaledTileSize));
        int lastX = min(mapWidth - 1, (int)std::ceil((canvasMax.x - origin.x) / scaledTileSize));
        int lastY = min(mapHeight - 1, (int)std::ceil((canvasMax.y - origin.y) / scaledTileSize));

        float startX = origin.x + firstX * scaledTileSize;
        float startY = origin.y + firstY * scaledTileSize;

        for (int y = firstY; y <= lastY; ++y)
        {
            float py = startY + (y - firstY) * scaledTileSize;

            for (int x = firstX; x <= lastX; ++x)
            {
                float px = startX + (x - firstX) * scaledTileSize;

                int tileVal = (int)tilemap->GetTile(x, mapHeight - 1 - y);
                ImU32 color = GetTileColor(tileVal);

                ImVec2 pMin(px, py);
                ImVec2 pMax(px + scaledTileSize, py + scaledTileSize);

                drawList->AddRectFilled(pMin, pMax, color);
                drawList->AddRect(pMin, pMax, IM_COL32(60, 60, 60, 255));
            }
        }

        for (auto *go : ObjectManager::loadedObjects)
        {
            if (!go || go->isPendingDestruction || !go->active || !go->transform)
                continue;

            float goX = go->transform->position.x;
            float goY = go->transform->position.y;

            float visualY = static_cast<float>(mapHeight) - goY;

            ImVec2 center(origin.x + goX * scaledTileSize, origin.y + visualY * scaledTileSize);
            float radius = scaledTileSize * 0.4f;

            ImVec2 pMin(center.x - radius, center.y - radius);
            ImVec2 pMax(center.x + radius, center.y + radius);

            if (ImGui::IsMouseHoveringRect(pMin, pMax))
            {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    ObjectManager::selectedObject = go;
                    ObjectManager::draggingObject = go;
                    currentTileValue = -1;
                }
            }

            ImU32 color = (ObjectManager::selectedObject == go) ? IM_COL32(255, 200, 0, 255) : IM_COL32(0, 255, 255, 255);
            drawList->AddCircleFilled(center, radius, color);
            drawList->AddCircle(center, radius, IM_COL32(0, 0, 0, 255), 0, 2.0f);

            ImVec2 textSize = ImGui::CalcTextSize(go->name.c_str());
            drawList->AddText(ImVec2(center.x - (textSize.x * 0.5f), center.y + radius + 2.0f), IM_COL32(255, 255, 255, 255), go->name.c_str());
        }

        if (ObjectManager::draggingObject)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImVec2 mousePos = ImGui::GetMousePos();

                float worldX = (mousePos.x - origin.x) / scaledTileSize;
                float worldY = static_cast<float>(mapHeight) - ((mousePos.y - origin.y) / scaledTileSize);

                ObjectManager::draggingObject->transform->position.x = std::clamp(worldX, 0.0f, static_cast<float>(mapWidth));
                ObjectManager::draggingObject->transform->position.y = std::clamp(worldY, 0.0f, static_cast<float>(mapHeight));
            }
            else
            {
                ObjectManager::draggingObject = nullptr;
            }
        }

        ImGui::SetCursorScreenPos(canvasPos);
        ImGui::InvisibleButton("CanvasDropTarget", canvasSize);

        bool canvasHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        bool canvasActive = ImGui::IsItemActive();

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DND_GAMEOBJECT"))
            {
                Engine::GameObject **droppedObj = (Engine::GameObject **)payload->Data;
                if (droppedObj && *droppedObj)
                {
                    ImVec2 mousePos = ImGui::GetMousePos();

                    float worldX = (mousePos.x - origin.x) / scaledTileSize;
                    float worldY = static_cast<float>(mapHeight) - ((mousePos.y - origin.y) / scaledTileSize);

                    (*droppedObj)->transform->position.x = std::clamp(worldX, 0.0f, static_cast<float>(mapWidth));
                    (*droppedObj)->transform->position.y = std::clamp(worldY, 0.0f, static_cast<float>(mapHeight));

                    ObjectManager::selectedObject = *droppedObj;
                }
            }
            ImGui::EndDragDropTarget();
        }

        if ((canvasHovered || canvasActive) && !ObjectManager::draggingObject && currentTileValue != -1)
        {
            ImVec2 mousePos = io.MousePos;
            int screenGridX = static_cast<int>(std::floor((mousePos.x - origin.x) / scaledTileSize));
            int screenGridY = static_cast<int>(std::floor((mousePos.y - origin.y) / scaledTileSize));

            if (screenGridX >= 0 && screenGridX < mapWidth && screenGridY >= 0 && screenGridY < mapHeight)
            {
                hoveredX = screenGridX;
                hoveredY = mapHeight - 1 - screenGridY;

                ImVec2 hMin = ImVec2(origin.x + (screenGridX * scaledTileSize), origin.y + (screenGridY * scaledTileSize));
                ImVec2 hMax = ImVec2(hMin.x + scaledTileSize, hMin.y + scaledTileSize);
                drawList->AddRect(hMin, hMax, IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);

                if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    ApplyPaint(hoveredX, hoveredY);
                }
            }
            else
            {
                hoveredX = -1;
                hoveredY = -1;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_S))
            {
                SaveMap("Assets/map.txt");
                SaveMap("Assets/tmpMap.txt");
            }
        }
        else
        {
            hoveredX = -1;
            hoveredY = -1;
        }
    }

    void GameEditor::ApplyPaint(int gridX, int gridY)
    {
        if (gridX < 0 || gridX >= mapWidth || gridY < 0 || gridY >= mapHeight)
            return;

        Engine::TileType newTile = static_cast<Engine::TileType>(currentTileValue);
        tilemap->SetTile(gridX, gridY, newTile);
    }

    void GameEditor::SaveMap(const char *filepath)
    {
        if (!tilemap)
            return;

        std::ofstream file(filepath, std::ios::out | std::ios::trunc);
        if (!file.is_open())
            return;

        for (int y = 0; y < mapHeight; ++y)
        {
            for (int x = 0; x < mapWidth; ++x)
            {
                int tile = static_cast<int>(tilemap->GetTile(x, y));
                char c = '0';

                switch (tile)
                {
                case 0:
                    c = '0';
                    break;
                case 1:
                    c = '1';
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