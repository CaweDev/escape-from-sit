/**
 * @file Editor/LevelEditor.cpp
 * @brief LevelEditor implementation
 */
#include "LevelEditor.hpp"
#include <iostream>
#include <fstream>

// Engine includes (Adjust paths to match your architecture)
#include "Engine/Scene/Scene.hpp"
#include "Engine/TileMap/TileMap.hpp"

namespace AEEditor
{
    Engine::Scene *LevelEditor::s_scene = nullptr;
    Engine::TileMap *LevelEditor::s_tileMap = nullptr;

    void LevelEditor::Initialize(Engine::Scene *scene, Engine::TileMap *tileMap)
    {
        s_scene = scene;
        s_tileMap = tileMap;
    }

    void LevelEditor::DrawAssetBrowser()
    {
        ImGui::Begin("Toolbar"); // Or rename to "Asset Browser"
        ImGui::Text("Drag and drop objects into the Scene");
        ImGui::Separator();

        // Object 1: Door
        ImGui::Button("Door Object (ID: 10)");
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            int prefabID = 10;
            ImGui::SetDragDropPayload("DND_PREFAB", &prefabID, sizeof(int));
            ImGui::Text("Dragging Door...");
            ImGui::EndDragDropSource();
        }

        // Object 2: Terminal
        ImGui::Button("Terminal Object (ID: 11)");
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            int prefabID = 11;
            ImGui::SetDragDropPayload("DND_PREFAB", &prefabID, sizeof(int));
            ImGui::Text("Dragging Terminal...");
            ImGui::EndDragDropSource();
        }

        ImGui::Separator();
        if (ImGui::Button("Save Map"))
        {
            SaveMap("Assets/map.txt");
        }

        ImGui::End();
    }

    void LevelEditor::HandleSceneDropTarget(float camOffsetX, float camOffsetY)
    {
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DND_PREFAB"))
            {
                IM_ASSERT(payload->DataSize == sizeof(int));
                int droppedID = *(const int *)payload->Data;

                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 windowPos = ImGui::GetWindowPos();

                // Calculate local window coordinates
                float localX = mousePos.x - windowPos.x;
                float localY = mousePos.y - windowPos.y;

                // Transform to engine world coordinates
                float worldX = localX - camOffsetX;
                float worldY = localY - camOffsetY;

                SpawnObject(droppedID, worldX, worldY);
            }
            ImGui::EndDragDropTarget();
        }
    }

    void LevelEditor::SpawnObject(int prefabID, float worldX, float worldY)
    {
        // Example grid snapping (assuming 1.0f or predefined tileSize)
        float tileSize = 64.0f; // Replace with actual tile size
        int tileX = static_cast<int>(worldX / tileSize);
        int tileY = static_cast<int>(worldY / tileSize);

        if (s_tileMap)
        {
            s_tileMap->SetTile(tileX, tileY, static_cast<Engine::TileType>(prefabID));
        }

        if (prefabID == 10)
            SpawnDoorAt(tileX, tileY);
        if (prefabID == 11)
            SpawnTerminalAt(tileX, tileY);
    }

    void LevelEditor::SpawnDoorAt(int x, int y)
    {
        std::cout << "[LevelEditor] Door dropped at grid (" << x << ", " << y << ")\n";
        // Execute original SpawnDoorAt logic here
    }

    void LevelEditor::SpawnTerminalAt(int x, int y)
    {
        std::cout << "[LevelEditor] Terminal dropped at grid (" << x << ", " << y << ")\n";
        // Execute original SpawnTerminalAt logic here
    }

    void LevelEditor::SaveMap(const char *filepath)
    {
        if (!s_tileMap)
            return;

        std::ofstream file(filepath);
        if (!file.is_open())
            return;

        for (int y = 0; y < s_tileMap->height; ++y)
        {
            for (int x = 0; x < s_tileMap->width; ++x)
            {
                int tile = (int)s_tileMap->GetTile(x, y);
                char c = '0';

                if (tile == 0)
                    c = '0';
                else if (tile == 1)
                    c = '1';
                else if (tile == 10)
                    c = 'D';
                else if (tile == 11)
                    c = 'T';
                else if (tile == 12)
                    c = 'E';
                else if (tile == 20)
                    c = 'K';
                else
                    c = '0';

                file << c;
            }
            file << '\n';
        }
        file.close();
        std::cout << "[LevelEditor] Saved map to " << filepath << "\n";
    }
}