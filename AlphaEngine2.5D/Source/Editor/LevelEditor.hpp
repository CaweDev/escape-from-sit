/**
 * @file Editor/LevelEditor.hpp
 * @brief ImGui-driven Level Editor subsystem
 */
#pragma once

#include "imgui.h"

namespace Engine
{
    class Scene;
    class TileMap;
}

namespace AEEditor
{
    class LevelEditor
    {
    public:
        static void Initialize(Engine::Scene *scene, Engine::TileMap *tileMap);
        static void DrawAssetBrowser();
        static void HandleSceneDropTarget(float camOffsetX, float camOffsetY);
        static void SaveMap(const char *filepath);

    private:
        static Engine::Scene *s_scene;
        static Engine::TileMap *s_tileMap;

        static void SpawnObject(int prefabID, float worldX, float worldY);
        static void SpawnDoorAt(int x, int y);
        static void SpawnTerminalAt(int x, int y);
    };
}