#include "CCTVPlacement.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iostream>
namespace Game
{
    static bool InBounds(int w, int h, int x, int y)
    {
        return (x >= 0 && y >= 0 && x < w && y < h);
    }

    static int Idx(int w, int x, int y)
    {
        return y * w + x;
    }

    static bool IsFloor(Engine::TileMap* map, int x, int y)
    {
        if (!map) return false;
        return map->IsWalkable((float)x + 0.5f, (float)y + 0.5f);
    }

    static bool IsWall(Engine::TileMap* map, int x, int y, int w, int h)
    {
        if (!map) return false;
        if (!InBounds(w, h, x, y)) return false; // OOB is not wall
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

    // corridor-ish: 2-open (straight) or 3-open (junction). avoid dead-ends (1-open).
    static bool IsCorridorTile(Engine::TileMap* map, int x, int y)
    {
        if (!IsFloor(map, x, y)) return false;
        int open = OpenNeighborCount(map, x, y);
        return (open == 2 || open >= 3);
    }

    // Determine if this tile is in a large open room (classroom) vs corridor
    // Rooms have more surrounding open space
    static bool IsLikelyRoom(Engine::TileMap* map, int x, int y, int w, int h)
    {
        if (!IsFloor(map, x, y)) return false;
        
        // Count open tiles in a 5x5 area around this tile
        int openCount = 0;
        for (int dy = -2; dy <= 2; ++dy)
        {
            for (int dx = -2; dx <= 2; ++dx)
            {
                int nx = x + dx;
                int ny = y + dy;
                if (InBounds(w, h, nx, ny) && IsFloor(map, nx, ny))
                {
                    openCount++;
                }
            }
        }
        
        // If more than 15 tiles in 5x5 area are open, it's likely a room
        // Corridors typically have 10-15 open tiles, rooms have 15-25
        return openCount > 15;
    }

    // Check if this is a good corridor surveillance point
    // Prioritize: junctions, turns, long straight corridors
    static int GetCorridorPriority(Engine::TileMap* map, int x, int y, int w, int h)
    {
        if (!IsFloor(map, x, y)) return 0;
        if (IsLikelyRoom(map, x, y, w, h)) return 0; // Skip rooms entirely
        
        int open = OpenNeighborCount(map, x, y);
        
        // Junction (3-4 way) = highest priority
        if (open >= 3) return 100;
        
        // Straight corridor (2 way) = medium priority
        if (open == 2)
        {
            // Check if it's a turn (L-shape)
            bool R = IsFloor(map, x + 1, y);
            bool L = IsFloor(map, x - 1, y);
            bool U = IsFloor(map, x, y + 1);
            bool D = IsFloor(map, x, y - 1);
            
            // L-shaped turn (higher priority)
            if ((R && U) || (R && D) || (L && U) || (L && D))
                return 80;
            
            // Straight corridor (lower priority)
            return 50;
        }
        
        // Dead end = low priority (but still place some)
        if (open == 1) return 30;
        
        return 0;
    }

    static float RotFromDir(int dx, int dy)
    {
        // convert grid direction to rotation (degrees)
        //  +x => 0, +y => 90, -x => 180, -y => 270
        if (dx == 1 && dy == 0)  return 0.0f;
        if (dx == -1 && dy == 0) return 180.0f;
        if (dx == 0 && dy == 1)  return 90.0f;
        return 270.0f;
    }

    // decide what direction the camera should face based on corridor shape
    static void ChooseFacingDir(Engine::TileMap* map, int x, int y, int& outDx, int& outDy)
    {
        // check which neighbors are open
        bool R = IsFloor(map, x + 1, y);
        bool L = IsFloor(map, x - 1, y);
        bool U = IsFloor(map, x, y + 1);
        bool D = IsFloor(map, x, y - 1);

        // straight horizontal corridor
        if (L && R && !U && !D) { outDx = 1; outDy = 0; return; }
        // straight vertical corridor
        if (U && D && !L && !R) { outDx = 0; outDy = 1; return; }

        // junction / corner: pick a "main" open direction (simple priority)
        if (R) { outDx = 1; outDy = 0; return; }
        if (U) { outDx = 0; outDy = 1; return; }
        if (L) { outDx = -1; outDy = 0; return; }
        outDx = 0; outDy = -1;
    }

    // place camera inside floor tile, pushed slightly toward any adjacent wall
    static bool PlaceOnWallInsideFloor(Engine::TileMap* map, int w, int h, int tx, int ty,
        float z, float eps, Engine::Vector3& outPos)
    {
        float fx = (float)tx + 0.5f;
        float fy = (float)ty + 0.5f;

        struct W { int wx, wy; float pushX, pushY; };
        // push back INTO floor (towards floor center) by eps
        W walls[4] =
        {
            { tx - 1, ty,  +eps,  0.0f }, // wall left -> push right
            { tx + 1, ty,  -eps,  0.0f }, // wall right -> push left
            { tx, ty - 1,  0.0f, +eps },  // wall down -> push up
            { tx, ty + 1,  0.0f, -eps },  // wall up -> push down
        };

        for (auto& c : walls)
        {
            if (IsWall(map, c.wx, c.wy, w, h))
            {
                outPos = Engine::Vector3(fx + c.pushX, fy + c.pushY, z);
                // safety clamp
                outPos.x = (std::max)(0.05f, (std::min)(outPos.x, (float)w - 0.05f));
                outPos.y = (std::max)(0.05f, (std::min)(outPos.y, (float)h - 0.05f));
                return true;
            }
        }

        // no adjacent wall found, just place at center
        outPos = Engine::Vector3(fx, fy, z);
        return true;
    }

    std::vector<CCTVInfo> GenerateCCTVsPerZone(
        Engine::TileMap* map,
        const std::vector<int>& zoneGrid,
        int w,
        int h,
        int perZone,
        int step,
        float minDist,
        float z,
        float eps
    )
    {
        std::vector<CCTVInfo> cams;
        if (!map) return cams;
        if ((int)zoneGrid.size() != w * h) return cams;

        // Define spawn exclusion zone (top-left corner area where player spawns)
        // Increased to 15x15 for guaranteed spawn safety
        const int spawnExcludeLeft = 0;
        const int spawnExcludeRight = 15;   // Increased from 8 to 15
        const int spawnExcludeBottom = 0;
        const int spawnExcludeTop = 15;     // Increased from 8 to 15

        std::cout << "[CCTV Placement] Spawn exclusion zone: (" << spawnExcludeLeft << "," << spawnExcludeBottom 
                  << ") to (" << spawnExcludeRight << "," << spawnExcludeTop << ") - Player spawns at (2, 2)\n";

        // Lambda to check if position is in spawn exclusion zone
        auto IsInSpawnZone = [&](int x, int y) -> bool {
            return (x >= spawnExcludeLeft && x <= spawnExcludeRight &&
                    y >= spawnExcludeBottom && y <= spawnExcludeTop);
        };

        // Collect ALL corridor candidates with priority scores per zone
        struct Candidate { int x, y, priority; };
        std::vector<Candidate> candidates[5]; // 1..4
        
        // Scan entire map for corridor tiles (not just stepped)
        for (int y = 1; y < h - 1; ++y)
        {
            for (int x = 1; x < w - 1; ++x)
            {
                // Skip spawn exclusion zone
                if (IsInSpawnZone(x, y)) continue;
                
                int zid = zoneGrid[Idx(w, x, y)];
                if (zid < 1 || zid > 4) continue;
                
                int priority = GetCorridorPriority(map, x, y, w, h);
                if (priority > 0)
                {
                    candidates[zid].push_back({ x, y, priority });
                }
            }
        }

        std::cout << "[CCTV Placement] Corridor candidates found per zone (excluding spawn area):\n";
        for (int zid = 1; zid <= 4; ++zid)
        {
            std::cout << "  Zone " << zid << ": " << candidates[zid].size() << " candidates\n";
        }

        // For each zone, place cameras prioritizing high-value locations
        for (int zid = 1; zid <= 4; ++zid)
        {
            // Sort by priority (highest first)
            std::sort(candidates[zid].begin(), candidates[zid].end(),
                [](const Candidate& a, const Candidate& b) {
                    return a.priority > b.priority;
                });

            int placed = 0;
            int attempts = 0;
            int maxAttempts = (int)candidates[zid].size();

            for (auto& cand : candidates[zid])
            {
                if (placed >= perZone) break;
                if (attempts >= maxAttempts) break;
                attempts++;

                int x = cand.x;
                int y = cand.y;

                // Double-check spawn zone exclusion (safety check)
                if (IsInSpawnZone(x, y)) {
                    std::cout << "  [Zone " << zid << "] SKIPPED camera at (" << x << "," << y 
                              << ") - in spawn zone!\n";
                    continue;
                }

                CCTVInfo info{};
                info.zoneId = zid;
                info.tileX = x;
                info.tileY = y;

                // Place position near wall inside floor tile
                if (!PlaceOnWallInsideFloor(map, w, h, x, y, z, eps, info.pos))
                    continue;

                // Face along corridor
                int dx = 1, dy = 0;
                ChooseFacingDir(map, x, y, dx, dy);
                info.rot = RotFromDir(dx, dy);

                // Min distance check (within same zone for better coverage)
                bool tooClose = false;
                for (auto& e : cams)
                {
                    if (e.zoneId == zid && DistSq2D(e.pos, info.pos) < minDist * minDist)
                    {
                        tooClose = true;
                        break;
                    }
                }
                if (tooClose) continue;

                cams.push_back(info);
                placed++;
                
                std::cout << "  [Zone " << zid << "] Placed CCTV #" << placed 
                          << " at (" << x << "," << y << ") priority=" << cand.priority << "\n";
            }

            std::cout << "[CCTV Placement] Zone " << zid << ": Placed " << placed << "/" << perZone << " cameras\n";

            // If still not enough cameras placed, try with reduced spacing
            if (placed < perZone)
            {
                std::cout << "[CCTV Placement] Zone " << zid << " trying with reduced spacing...\n";
                float reducedMinDist = minDist * 0.65f; // Reduce to 65% of original spacing
                
                for (auto& cand : candidates[zid])
                {
                    if (placed >= perZone) break;
                    
                    int x = cand.x;
                    int y = cand.y;

                    // Double-check spawn zone exclusion (safety check)
                    if (IsInSpawnZone(x, y)) {
                        std::cout << "  [Zone " << zid << "] SKIPPED camera at (" << x << "," << y 
                                  << ") - in spawn zone (reduced spacing)!\n";
                        continue;
                    }

                    CCTVInfo info{};
                    info.zoneId = zid;
                    info.tileX = x;
                    info.tileY = y;

                    PlaceOnWallInsideFloor(map, w, h, x, y, z, eps, info.pos);

                    // Check with reduced min distance
                    bool tooClose = false;
                    for (auto& e : cams)
                    {
                        if (e.zoneId == zid && DistSq2D(e.pos, info.pos) < reducedMinDist * reducedMinDist)
                        {
                            tooClose = true;
                            break;
                        }
                    }
                    if (tooClose) continue;

                    int dx = 1, dy = 0;
                    ChooseFacingDir(map, x, y, dx, dy);
                    info.rot = RotFromDir(dx, dy);

                    cams.push_back(info);
                    placed++;
                    std::cout << "  [Zone " << zid << "] Placed CCTV #" << placed 
                              << " (reduced spacing) at (" << x << "," << y << ")\n";
                }
            }
        }

        std::cout << "[CCTV Placement] Total cameras placed: " << cams.size() << "\n";
        return cams;
    }

} // namespace Game