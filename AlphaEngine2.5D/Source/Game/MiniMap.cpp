#include "MiniMap.hpp"
#include "Engine/Core/GameObject.hpp"
#include <cmath>
#include <algorithm>

namespace Game {

    static float DegToRad(float d) { return d * 3.1415926f / 180.0f; }

    Engine::Vector2 MiniMap::Forward(float rotDeg) const {
        float a = DegToRad(rotDeg);
        return Engine::Vector2(std::cos(a), std::sin(a));
    }

    Engine::Vector2 MiniMap::Right(float rotDeg) const {
        float a = DegToRad(rotDeg);
        return Engine::Vector2(-std::sin(a), std::cos(a));
    }

    void MiniMap::Initialize(Engine::Scene* s, Engine::TileMap* m, Engine::Transform* p, Engine::Transform* g) {
        scene = s;
        map = m;
        playerTf = p;
        guardTf = g;

        if (!scene || !map || !playerTf) return;

        // Use ANY existing sprite you already have (no new assets)
        sharedSprite = std::make_shared<Engine::Sprite>();
        sharedSprite->Load("Assets/PlanetTexture.png", 8, 8);

        BuildGrid();
    }

    void MiniMap::SetGuard(Engine::Transform* g) {
        guardTf = g;
    }

    void MiniMap::BuildGrid() {
        cells.clear();

        // force odd
        if (gridSize < 5) gridSize = 5;
        if ((gridSize % 2) == 0) gridSize += 1;

        int half = gridSize / 2;

        cells.reserve(gridSize * gridSize);

        for (int dy = -half; dy <= half; ++dy) {
            for (int dx = -half; dx <= half; ++dx) {

                auto go = scene->CreateGameObject("MM_Cell");
                go->transform->localScale = Engine::Vector2(cellScale, cellScale);

                auto sr = go->AddComponent<Engine::SpriteRenderer>();
                sr->SetSprite(sharedSprite);
                sr->billboard = true;
                sr->sortingOrder = 9000; // super on top

                cells.push_back({ go, sr, dx, dy });
            }
        }
    }

    void MiniMap::Update() {
        if (!scene || !map || !playerTf) return;
        UpdateGrid();
    }

    void MiniMap::UpdateGrid() {
        int pTileX = (int)playerTf->position.x;
        int pTileY = (int)playerTf->position.y;

        // HUD anchor in world space (in front-right of player)
        Engine::Vector2 f = Forward(playerTf->rotation);
        Engine::Vector2 r = Right(playerTf->rotation);

        Engine::Vector3 anchor(
            playerTf->position.x + f.x * hudForward + r.x * hudRight,
            playerTf->position.y + f.y * hudForward + r.y * hudRight,
            hudHeightZ
        );

        // minimap axes (right = +x on minimap, forward = -y on minimap for "top-down feel")
        // we place the grid on a little "panel" facing the camera by using r and f
        for (auto& c : cells) {
            int tx = pTileX + c.dx;
            int ty = pTileY + c.dy;

            // position each cell relative to anchor using right/forward
            float sx = (float)c.dx * cellScale * 1.35f;
            float sy = (float)c.dy * cellScale * 1.35f;

            c.go->transform->position = Engine::Vector3(
                anchor.x + r.x * sx + f.x * (-sy),
                anchor.y + r.y * sx + f.y * (-sy),
                hudHeightZ
            );

            // outside map = treat as wall
            bool inBounds = (tx >= 0 && tx < map->width && ty >= 0 && ty < map->height);
            bool walkable = false;
            if (inBounds) walkable = map->IsWalkable(tx + 0.5f, ty + 0.5f);

            // colors:
            // NOTE: if your SpriteRenderer doesn't have .color, change to whatever your engine uses.
            if (!inBounds || !walkable) {
                c.sr->color = Engine::Color(0.08f, 0.08f, 0.08f, 1.0f); // wall
            }
            else {
                c.sr->color = Engine::Color(0.35f, 0.35f, 0.35f, 1.0f); // floor
            }

            // player center
            if (c.dx == 0 && c.dy == 0) {
                c.sr->color = Engine::Color(0.10f, 1.0f, 0.10f, 1.0f);
            }

            // guard dot if within window
            if (guardTf) {
                int gTileX = (int)guardTf->position.x;
                int gTileY = (int)guardTf->position.y;
                if (tx == gTileX && ty == gTileY) {
                    c.sr->color = Engine::Color(1.0f, 0.20f, 0.20f, 1.0f);
                }
            }
        }
    }

}
