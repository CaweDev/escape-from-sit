/**
 * @file Game Editor.hpp
 * @brief Game editor
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
    class GameEditor
    {
    public:
        static int currentTileValue;

        static void Initialize(Engine::Scene *scene, Engine::TileMap *tileMap);
        static void Exit();

        static void DrawAssetBrowser();
        static void DrawMapEditorViewport();
        static void SaveMap(const char *filepath);

        static bool Is3DMode();

    private:
        static Engine::Scene *scene;
        static Engine::TileMap *tilemap;

        static bool is3DMode;

        static int mapWidth;
        static int mapHeight;
        static float tileSize;

        static ImVec2 scrolling;
        static float zoom;

        static int hoveredX;
        static int hoveredY;

        static void ApplyPaint(int gridX, int gridY);
        static ImU32 GetTileColor(int tileValue);
    };
}