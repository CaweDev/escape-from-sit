#include "CCTVPlacement.hpp"
#include <cmath>

namespace Game
{
    static bool InBounds(Engine::TileMap* map, int x, int y)
    {
        if (!map) return false;
        return (x >= 0 && y >= 0 && x < map->width && y < map->height);
    }

    static bool IsFloor(Engine::TileMap* map, int x, int y)
    {
        if (!map) return false;
        if (!InBounds(map, x, y)) return false; // IMPORTANT
        return map->IsWalkable((float)x + 0.5f, (float)y + 0.5f);
    }

    static bool IsWall(Engine::TileMap* map, int x, int y)
    {
        if (!map) return false;
        if (!InBounds(map, x, y)) return false; // IMPORTANT: OOB is NOT wall
        return !map->IsWalkable((float)x + 0.5f, (float)y + 0.5f);
    }

    static int OpenNeighborCount(Engine::TileMap* map, int x, int y)
    {
        int c = 0;
        if (IsFloor(map, x + 1, y)) ++c;
        if (IsFloor(map, x - 1, y)) ++c;
        if (IsFloor(map, x, y + 1)) ++c;
        if (IsFloor(map, x, y - 1)) ++c;
        return c;
    }

    static float DistSq2D(const Engine::Vector3& a, const Engine::Vector3& b)
    {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

    static float RotFromDir(float dx, float dy)
    {
       
        if (std::fabs(dx) > std::fabs(dy))
            return (dx > 0.0f) ? 0.0f : 180.0f;
        else
            return (dy > 0.0f) ? 90.0f : 270.0f;
    }

    static bool PlaceOnWallFacingTile(Engine::TileMap* map, int tx, int ty,
        float z, float eps, CCTVInfo& out)
    {
        if (!map) return false;

        // floor center
        float fx = (float)tx + 0.5f;
        float fy = (float)ty + 0.5f;

        struct Cand { int wx, wy; float rot; float nx, ny; };
        // nx,ny = direction from FLOOR -> WALL (so we offset toward wall)
        Cand cand[4] =
        {
            { tx - 1, ty,    0.0f,  -1.0f,  0.0f }, 
            { tx + 1, ty,  180.0f,  +1.0f,  0.0f }, // wa
            { tx, ty - 1,   90.0f,   0.0f, -1.0f }, // wall below, camera faces +y (up)
            { tx, ty + 1,  270.0f,   0.0f, +1.0f }, // wall above, camera faces -y (down)
        };

        for (auto& c : cand)
        {
            if (!IsWall(map, c.wx, c.wy)) continue;
            out.pos = Engine::Vector3(
                fx + c.nx * eps,
                fy + c.ny * eps,
                z
            );
            out.rot = c.rot;
            return true;
        }

        return false;
    }

    std::vector<CCTVInfo> GenerateCCTVs(
        Engine::TileMap* map,
        int mapWidth,
        int mapHeight,
        int maxCctv,
        int step,
        float minDist,
        float z,
        float eps
    )
    {
        std::vector<CCTVInfo> cams;
        if (!map) return cams;
        cams.reserve(maxCctv);

        for (int y = 1; y < mapHeight - 1 && (int)cams.size() < maxCctv; y += step)
        {
            for (int x = 1; x < mapWidth - 1 && (int)cams.size() < maxCctv; x += step)
            {
                if (!IsFloor(map, x, y)) continue;

                int open = OpenNeighborCount(map, x, y);

                bool good = (open >= 3);
                if (!good && open == 2)
                {
                    if (((x + y) % (step * 2)) == 0) good = true;
                }
                if (!good) continue;

                CCTVInfo info{};
                if (!PlaceOnWallFacingTile(map, x, y, z, eps, info))
                    continue;

                bool tooClose = false;
                for (auto& e : cams)
                {
                    if (DistSq2D(e.pos, info.pos) < minDist * minDist)
                    {
                        tooClose = true;
                        break;
                    }
                }
                if (tooClose) continue;

                cams.push_back(info);
            }
        }

        return cams;
    }
}
