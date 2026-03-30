/**
 * @file EditorScene.hpp
 * @brief Editor rendering scene definition
 */
#pragma once

#include "Engine/Scene/Scene.hpp"
#include <vector>
#include <string>

// Forward Declarations
namespace Engine
{
    class Camera;
    class Transform;
    class TileMap;
    class TileMapRenderer;
}

namespace Game
{

    class EditorScene : public Engine::Scene
    {
    public:
        EditorScene();

        Engine::Camera *editorCamera;
        Engine::Transform *cameraTf;

        int mapWidth;
        int mapHeight;

        struct Zone
        {
            int id;
            int minX, maxX;
            int minY, maxY;
        };
        std::vector<Zone> zones;

        int GetZoneId(int tx, int ty) const;

        void OnLoad() override;
        void OnUpdate() override;
        void OnUnload() override;

    private:
        void SpawnExistingDoors(Engine::TileMap *map, Engine::TileMapRenderer *renderer);
        void SpawnExistingTerminals(Engine::TileMap *map);
        void SpawnExistingCCTVs(Engine::TileMap *map);
    };

} // namespace Game