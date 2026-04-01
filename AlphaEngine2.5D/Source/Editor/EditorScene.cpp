/*!
 * \file EditorScene.cpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief EditorScene implementation and helper spawners for editor mode
 */
#include "imgui.h"
#include "EditorScene.hpp"
#include "ObjectManager.hpp"

#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Graphics/TileMapRenderer.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Physics/TileMapCollider2D.hpp"
#include "Engine/Physics/BoxCollider2D.hpp"
#include "Engine/Core/AssetManager.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Graphics/DoorRenderer.hpp"
#include "Engine/TileMap/Door.hpp"
#include "Engine/Interaction/Interactable.hpp"

// Required for free camera input polling
#include "Engine/Input/Input.hpp"
#include "Engine/Input/KeyCode.hpp"
#include "Engine/Core/Time.hpp"

#include "Editor/GameEditor.hpp"
#include "Game/AccessCard/AccessCard.hpp"
#include "Game/CCTV/CCTVCamera.hpp"

#include <fstream>
#include <cmath>
#include <iostream>

namespace Game
{
    using namespace Engine;

    static constexpr float spawnExcludeX = 2.5f;
    static constexpr float spawnExcludeY = 2.5f;
    static constexpr float spawnExcludeRadius = 5.0f;

    /*! \brief Construct an EditorScene used for in-engine editing. */
    EditorScene::EditorScene() : Scene("EditorScene"), editorCamera(nullptr), cameraTf(nullptr), mapWidth(0), mapHeight(0) {}

    /*! \brief Determine zone id for a given tile coord. \param tx [in] Tile X \param ty [in] Tile Y \return zone id or -1 */
    int EditorScene::GetZoneId(int tx, int ty) const
    {
        for (auto const &z : zones)
        {
            if (tx >= z.minX && tx <= z.maxX &&
                ty >= z.minY && ty <= z.maxY)
            {
                return z.id;
            }
        }
        return -1;
    }

    /*! \brief Load editor scene map, spawn existing objects for editing. */
    void EditorScene::OnLoad()
    {
        std::vector<std::string> mapRows;
        {
            std::ifstream file("Assets/tmpMap.txt");
            if (!file.is_open())
            {
                file.clear();
                file.open("Assets/map.txt");
            }

            if (!file.is_open())
                return;

            std::string line;
            while (std::getline(file, line))
            {
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                if (!line.empty())
                    mapRows.push_back(line);
            }
            file.close();
            if (mapRows.empty())
                return;
        }

        mapHeight = (int)mapRows.size();
        mapWidth = (int)mapRows[0].size();

        zones.clear();
        int midX = mapWidth / 2;
        int midY = mapHeight / 2;

        zones.push_back({1, 0, midX - 1, 0, midY - 1});
        zones.push_back({2, 0, midX - 1, midY, mapHeight - 1});
        zones.push_back({3, midX, mapWidth - 1, midY, mapHeight - 1});
        zones.push_back({4, midX, mapWidth - 1, 0, midY - 1});

        std::vector<int> worldMap(mapWidth * mapHeight, 0);
        for (int y = 0; y < mapHeight; ++y)
        {
            for (int x = 0; x < mapWidth; ++x)
            {
                char c = mapRows[y][x];
                int tileValue = 0;

                if (c == '0')
                    tileValue = 0;
                else if (c == '1')
                    tileValue = 1;
                else if (c == 'D')
                    tileValue = 10;
                else if (c == 'T')
                    tileValue = 11;
                else if (c == 'E')
                    tileValue = 12;
                else if (c == 'C')
                    tileValue = 13;
                else if (c == 'K')
                    tileValue = 20;
                else if (c == 'F')
                    tileValue = 15;
                else if (c == 'G')
                    tileValue = 16;
                else if (c == 'H')
                    tileValue = 17;
                else if (c == 'J')
                    tileValue = 18;
                else
                    tileValue = c - '0';    

                worldMap[y * mapWidth + x] = tileValue;
            }
        }

        auto world = CreateGameObject("World");
        auto map = world->AddComponent<TileMap>();
        map->LoadFromArray(worldMap.data(), mapWidth, mapHeight);

        auto renderer = world->AddComponent<TileMapRenderer>();
        renderer->wallColorNS = Color(0.8f, 0.2f, 0.2f, 1.0f);
        renderer->wallColorEW = Color(0.6f, 0.15f, 0.15f, 1.0f);
        renderer->ceilingColor = Color(0.1f, 0.1f, 0.2f, 1.0f);
        renderer->floorColor = Color(0.3f, 0.25f, 0.2f, 1.0f);

        renderer->SetCeilingSprite(AssetManager::GetSprite("Assets/Tilemap/Wallpaper_02_White_1.png"));
        renderer->SetFloorSprite(AssetManager::GetSprite("Assets/Tilemap/s2 Concrete Squares 2 Grey.png"));
        renderer->SetWallSprite(AssetManager::GetSprite("Assets/Tilemap/Concrete_01_White_1.png"));
        renderer->SetDoorSprite(AssetManager::GetSprite("Assets/Tilemap/school_sliding_door.png"));

        auto tileMapCollider = world->AddComponent<TileMapCollider2D>();
        tileMapCollider->minHeight = 0.0f;
        tileMapCollider->maxHeight = 3.0f;

        auto camObj = CreateGameObject("EditorCamera");
        editorCamera = camObj->AddComponent<Camera>();

        editorCamera->backgroundColor = Color(0, 0, 0, 1);
        editorCamera->fogEnabled = true;
        editorCamera->fogDistance = 2.f;
        editorCamera->fogMinVisibility = -0.1f;
        editorCamera->fogMaxVisibility = 0.6f;

        cameraTf = camObj->transform;

        cameraTf->position = Vector3(2.5f, 2.5f, 0.5f);
        cameraTf->rotation = 0.0f;

        AEEditor::GameEditor::Initialize(this, map);

        // std::ifstream jsonFile("Assets/Config/Scenes/EditorScene.json");
        // if (jsonFile.is_open())
        // {
        //     jsonFile.close();
        //     AEEditor::ObjectManager::LoadSceneFromJson("Assets/Config/Scenes/EditorScene.json");
        // }
        // else
        // {
        // SpawnExistingDoors(map, renderer);
        // SpawnExistingTerminals(map);
        // SpawnExistingCCTVs(map);
        // }
    }

    /*! \brief Per-frame update handling editor camera controls. */
    void EditorScene::OnUpdate()
    {
        {
            if (!cameraTf)
                return;

            if (!AEEditor::GameEditor::Is3DMode())
                return;

            float moveSpeed = 4.0f;
            float dt = Engine::Time::deltaTime;

            if (Engine::Input::GetKey(Engine::KeyCode::LeftShift))
            {
                moveSpeed *= 2.5f;
            }

            static bool isDraggingCamera = false;
            static ImVec2 lockedCursorPos;

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                isDraggingCamera = true;
                lockedCursorPos = ImGui::GetMousePos();
            }
            else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            {
                isDraggingCamera = false;
                ImGui::GetIO().WantSetMousePos = true;
                ImGui::GetIO().MousePos = lockedCursorPos;
            }

            if (isDraggingCamera)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_None);
                float mouseDeltaX = ImGui::GetIO().MouseDelta.x;
                float sensitivity = 0.07f;
                cameraTf->rotation += mouseDeltaX * sensitivity;

                if (cameraTf->rotation >= 360.0f)
                    cameraTf->rotation -= 360.0f;
                if (cameraTf->rotation < 0)
                    cameraTf->rotation += 360.0f;
            }

            float rot = AEDegToRad(cameraTf->rotation);
            float fwdX = std::cos(rot);
            float fwdY = std::sin(rot);

            float rightX = std::cos(rot - HALF_PI);
            float rightY = std::sin(rot - HALF_PI);

            if (Engine::Input::GetKey(Engine::KeyCode::W))
            {
                cameraTf->position.x += fwdX * moveSpeed * dt;
                cameraTf->position.y += fwdY * moveSpeed * dt;
            }
            if (Engine::Input::GetKey(Engine::KeyCode::S))
            {
                cameraTf->position.x -= fwdX * moveSpeed * dt;
                cameraTf->position.y -= fwdY * moveSpeed * dt;
            }
            if (Engine::Input::GetKey(Engine::KeyCode::A))
            {
                cameraTf->position.x -= rightX * moveSpeed * dt;
                cameraTf->position.y -= rightY * moveSpeed * dt;
            }
            if (Engine::Input::GetKey(Engine::KeyCode::D))
            {
                cameraTf->position.x += rightX * moveSpeed * dt;
                cameraTf->position.y += rightY * moveSpeed * dt;
            }

            if (Engine::Input::GetKey(Engine::KeyCode::Space))
                cameraTf->position.z += moveSpeed * dt;
            if (Engine::Input::GetKey(Engine::KeyCode::LeftControl))
                cameraTf->position.z -= moveSpeed * dt;

            if (cameraTf->position.z < 0.2f)
                cameraTf->position.z = 0.2f;
            if (cameraTf->position.z > 2.8f)
                cameraTf->position.z = 2.8f;
        }
    }

    /*! \brief Unload editor scene (no-op). */
    void EditorScene::OnUnload() {}




} // namespace Game