/**
 * @file Editor/LevelEditor.hpp
 * @brief ImGui-driven 2D Grid Level Editor
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
        static void Exit();

        static void DrawAssetBrowser();
        static void DrawMapEditorViewport();
        static void SaveMap(const char *filepath);

        static bool Is3DMode();

    private:
        static Engine::Scene *s_scene;
        static Engine::TileMap *s_tileMap;

        static int currentTileValue;
        static bool is3DMode;

        static int mapWidth;
        static int mapHeight;
        static float tileSize;

        static ImVec2 scrolling;
        static float zoom;

        static int hoveredX;
        static int hoveredY;

        static void ApplyPaint(int gridX, int gridY);
        static void SpawnDoorAt(int x, int y);
        static void SpawnTerminalAt(int x, int y);
        static ImU32 GetTileColor(int tileValue);
    };
}