#pragma once

#include "Engine/Scene/Scene.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Core/Transform.hpp"
#include <Engine/Graphics/SpriteRenderer.hpp>
#include <Engine/Graphics/Sprite.hpp>

#include <vector>
#include <memory>

namespace Game {

    class MiniMap {
    public:
        void Initialize(
            Engine::Scene* scene,
            Engine::TileMap* map,
            Engine::Transform* playerTf,
            Engine::Transform* guardTf = nullptr
        );

        void Update();
        void SetGuard(Engine::Transform* guardTf);

        // how many tiles wide/high for minimap (must be odd)
        int gridSize = 21; // 21x21 window
        float cellScale = 0.018f; // size of each cell sprite
        float hudHeightZ = 1.45f; // vertical height of minimap (near eye level)
        float hudForward = 0.85f; // in front of player
        float hudRight = 0.65f;   // to the right of player

    private:
        Engine::Scene* scene = nullptr;
        Engine::TileMap* map = nullptr;
        Engine::Transform* playerTf = nullptr;
        Engine::Transform* guardTf = nullptr;

        std::shared_ptr<Engine::Sprite> sharedSprite;

        struct Cell {
            Engine::GameObject* go = nullptr;
            Engine::SpriteRenderer* sr = nullptr;
            int dx = 0; // offset from player tile x
            int dy = 0; // offset from player tile y
        };

        std::vector<Cell> cells;

        Engine::Vector2 Forward(float rotDeg) const;
        Engine::Vector2 Right(float rotDeg) const;

        void BuildGrid();
        void UpdateGrid();
    };

}
