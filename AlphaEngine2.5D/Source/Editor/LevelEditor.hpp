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
        static void SaveMapTxt(const char *filepath);

        static bool Is3DMode();

    private:
        static Engine::Scene *s_scene;
        static Engine::TileMap *s_tileMap;

        static int s_currentTileValue;
        static bool s_is3DMode;

        static int s_mapWidth;
        static int s_mapHeight;
        static float s_tileSize;

        static ImVec2 s_scrolling;
        static float s_zoom;

        static int s_hoveredX;
        static int s_hoveredY;

        static void ApplyPaint(int gridX, int gridY);
        static void SpawnDoorAt(int x, int y);
        static void SpawnTerminalAt(int x, int y);
        static ImU32 GetTileColor(int tileValue);
    };
}