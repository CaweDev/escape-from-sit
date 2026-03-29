/*!
 * \file AlphaEngine2.5D\Source\Game\CCTV\CCTVPlacement.cpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief [AUTO] Added Doxygen file header
 */

#include "CCTVPlacement.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <queue>
#include <unordered_set>

/*!
 * \file CCTVPlacement.cpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief Implementation of CCTV placement heuristics and helpers
 */

namespace
{
    /*! \struct CCTVPlacementConfig
     *  \brief JSON-backed tuning parameters for CCTV placement
     */
    struct CCTVPlacementConfig : public Engine::IConfig
    {
        int roomOpenThreshold5x5 = 15;
        int enclosedAreaLimit = 2000;
        int guardedAreaLimit = 2000;

        int spawnExcludeLeft = 0;
        int spawnExcludeRight = 15;
        int spawnExcludeBottom = 0;
        int spawnExcludeTop = 15;

        int junctionPriority = 100;
        int turnPriority = 80;
        int straightPriority = 50;
        int deadEndPriority = 30;

        float placementClampMargin = 0.05f;

        /*! \brief Load placement tuning values from JSON. \param j [in] JSON node */
        void FromJson(const Json::Value& j) override
        {
            roomOpenThreshold5x5 = j.get("roomOpenThreshold5x5", roomOpenThreshold5x5).asInt();
            enclosedAreaLimit = j.get("enclosedAreaLimit", enclosedAreaLimit).asInt();
            guardedAreaLimit = j.get("guardedAreaLimit", guardedAreaLimit).asInt();

            spawnExcludeLeft = j.get("spawnExcludeLeft", spawnExcludeLeft).asInt();
            spawnExcludeRight = j.get("spawnExcludeRight", spawnExcludeRight).asInt();
            spawnExcludeBottom = j.get("spawnExcludeBottom", spawnExcludeBottom).asInt();
            spawnExcludeTop = j.get("spawnExcludeTop", spawnExcludeTop).asInt();

            junctionPriority = j.get("junctionPriority", junctionPriority).asInt();
            turnPriority = j.get("turnPriority", turnPriority).asInt();
            straightPriority = j.get("straightPriority", straightPriority).asInt();
            deadEndPriority = j.get("deadEndPriority", deadEndPriority).asInt();

            placementClampMargin = j.get("placementClampMargin", placementClampMargin).asFloat();
        }
    };

    CCTVPlacementConfig g_cctvPlacementConfig;
    bool g_cctvPlacementConfigLoaded = false;

    const CCTVPlacementConfig& GetCCTVPlacementConfig()
    {
        if (!g_cctvPlacementConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/cctv_placement.json", g_cctvPlacementConfig);
            if (g_cctvPlacementConfig.enclosedAreaLimit < 1) g_cctvPlacementConfig.enclosedAreaLimit = 1;
            if (g_cctvPlacementConfig.guardedAreaLimit < 1) g_cctvPlacementConfig.guardedAreaLimit = 1;
            if (g_cctvPlacementConfig.placementClampMargin < 0.0f) g_cctvPlacementConfig.placementClampMargin = 0.0f;
            g_cctvPlacementConfigLoaded = true;
        }
        return g_cctvPlacementConfig;
    }
}

namespace Game
{
    /*! \brief Check if coordinates are within map bounds. */
    static bool InBounds(int w, int h, int x, int y)
    {
        return (x >= 0 && y >= 0 && x < w && y < h);
    }

    /*! \brief Index helper into flat grid array. */
    static int Idx(int w, int x, int y)
    {
        return y * w + x;
    }

    /*! \brief True if tile at x,y is walkable floor. */
    static bool IsFloor(Engine::TileMap* map, int x, int y)
    {
        if (!map) return false;
        return map->IsWalkable((float)x + 0.5f, (float)y + 0.5f);
    }

    /*! \brief True if tile at x,y is a wall (non-walkable). */
    static bool IsWall(Engine::TileMap* map, int x, int y, int w, int h)
    {
        if (!map) return false;
        if (!InBounds(w, h, x, y)) return false; // OOB is not wall
        return !map->IsWalkable((float)x + 0.5f, (float)y + 0.5f);
    }

    /*! \brief Count floor neighbors (4-way) around tile. */
    static int OpenNeighborCount(Engine::TileMap* map, int x, int y)
    {
        int c = 0;
        if (IsFloor(map, x + 1, y)) ++c;
        if (IsFloor(map, x - 1, y)) ++c;
        if (IsFloor(map, x, y + 1)) ++c;
        if (IsFloor(map, x, y - 1)) ++c;
        return c;
    }

    /*! \brief Squared distance between two 2D positions (ignores z). */
    static float DistSq2D(const Engine::Vector3& a, const Engine::Vector3& b)
    {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

    // corridor-ish: 2-open (straight) or 3-open (junction). avoid dead-ends (1-open).
    /*! \brief Heuristic: is this tile corridor-like (2 or 3+ open neighbors). */
    static bool IsCorridorTile(Engine::TileMap* map, int x, int y)
    {
        if (!IsFloor(map, x, y)) return false;
        int open = OpenNeighborCount(map, x, y);
        return (open == 2 || open >= 3);
    }

    // Determine if this tile is in a large open room (classroom) vs corridor
    // Rooms have more surrounding open space
    /*! \brief Estimate whether a tile lies in a room (open area) vs corridor. */
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
        return openCount > GetCCTVPlacementConfig().roomOpenThreshold5x5;
    }

    // Determine if the floor region containing (sx,sy) is enclosed by walls/doors/markers
    // Do a bounded flood-fill; if the area is large (>AREA_LIMIT) consider it not enclosed
    /*! \brief Flood-fill to determine if region is enclosed by walls/markers. */
    static bool IsEnclosedByWallsAndMarkers(Engine::TileMap* map, int sx, int sy, int w, int h)
    {
        if (!IsFloor(map, sx, sy)) return false;
        const int AREA_LIMIT = GetCCTVPlacementConfig().enclosedAreaLimit; // if region larger than this, treat as open

        std::vector<std::vector<char>> seen(h, std::vector<char>(w, 0));
        std::queue<std::pair<int,int>> q;
        q.push({sx, sy});
        seen[sy][sx] = 1;
        int area = 0;

        std::vector<std::pair<int,int>> perimeter;

        while (!q.empty()) {
            auto p = q.front(); q.pop();
            ++area;
            if (area > AREA_LIMIT) return false;

            const int ox[4] = {1,-1,0,0};
            const int oy[4] = {0,0,1,-1};
            for (int k = 0; k < 4; ++k) {
                int nx = p.first + ox[k];
                int ny = p.second + oy[k];
                if (!InBounds(w, h, nx, ny)) continue;
                if (IsFloor(map, nx, ny)) {
                    if (!seen[ny][nx]) { seen[ny][nx] = 1; q.push({nx, ny}); }
                } else {
                    perimeter.push_back({nx, ny});
                }
            }
        }

        // dedupe perimeter
        std::sort(perimeter.begin(), perimeter.end());
        perimeter.erase(std::unique(perimeter.begin(), perimeter.end()), perimeter.end());

        // gather marker positions (tile coords) for terminals/exits/control room doors and spawned doors
        std::unordered_set<int> markerTiles; // key = y*w + x
        auto terms = map->GetTerminalPositions();
        for (auto &pos : terms) markerTiles.insert(((int)std::floor(pos.y)) * w + (int)std::floor(pos.x));
        auto exits = map->GetExitPositions();
        for (auto &pos : exits) markerTiles.insert(((int)std::floor(pos.y)) * w + (int)std::floor(pos.x));
        auto ctrls = map->GetControlRoomDoorPositions();
        for (auto &pos : ctrls) markerTiles.insert(((int)std::floor(pos.y)) * w + (int)std::floor(pos.x));

        // spawned doors (use positions cache)
        for (auto &pos : map->GetSpawnedDoorPositionsPublic()) {
            int tx = (int)std::floor(pos.x); int ty = (int)std::floor(pos.y);
            if (tx >= 0 && tx < w && ty >= 0 && ty < h) markerTiles.insert(ty * w + tx);
        }

        // Check every perimeter tile: must be a wall or one of the markers
        for (auto &pp : perimeter) {
            int px = pp.first, py = pp.second;
            if (IsWall(map, px, py, w, h)) continue;
            int key = py * w + px;
            if (markerTiles.find(key) != markerTiles.end()) continue;
            // If perimeter tile is walkable but not marked, it's an opening -> not enclosed
            if (IsFloor(map, px, py)) return false;
            // Unknown non-floor type (e.g., open tile) treat as not enclosed
            return false;
        }

        return true;
    }

    // Determine if the floor region containing (sx,sy) has any doors on its perimeter
    // If so, treat it as guarded (rooms with doors) so we can avoid placing CCTVs there.
    /*! \brief Flood-fill to determine if region perimeter contains doors (guarded). */
    static bool RegionGuardedByDoor(Engine::TileMap* map, int sx, int sy, int w, int h)
    {
        if (!IsFloor(map, sx, sy)) return false;
        const int AREA_LIMIT = GetCCTVPlacementConfig().guardedAreaLimit;

        std::vector<std::vector<char>> seen(h, std::vector<char>(w, 0));
        std::queue<std::pair<int,int>> q;
        q.push({sx, sy});
        seen[sy][sx] = 1;
        int area = 0;

        std::vector<std::pair<int,int>> perimeter;

        while (!q.empty()) {
            auto p = q.front(); q.pop();
            ++area;
            if (area > AREA_LIMIT) return false;

            const int ox[4] = {1,-1,0,0};
            const int oy[4] = {0,0,1,-1};
            for (int k = 0; k < 4; ++k) {
                int nx = p.first + ox[k];
                int ny = p.second + oy[k];
                if (!InBounds(w, h, nx, ny)) continue;
                if (IsFloor(map, nx, ny)) {
                    if (!seen[ny][nx]) { seen[ny][nx] = 1; q.push({nx, ny}); }
                } else {
                    perimeter.push_back({nx, ny});
                }
            }
        }

        if (perimeter.empty()) return false;

        // build set of door tile keys (y*w + x)
        std::unordered_set<int> doorKeys;
        for (auto &pos : map->GetSpawnedDoorPositionsPublic()) {
            int tx = (int)std::floor(pos.x); int ty = (int)std::floor(pos.y);
            if (tx >= 0 && tx < w && ty >= 0 && ty < h) doorKeys.insert(ty * w + tx);
        }
        for (auto &pos : map->GetControlRoomDoorPositions()) {
            int tx = (int)std::floor(pos.x); int ty = (int)std::floor(pos.y);
            if (tx >= 0 && tx < w && ty >= 0 && ty < h) doorKeys.insert(ty * w + tx);
        }
        for (auto &pos : map->GetExitPositions()) {
            int tx = (int)std::floor(pos.x); int ty = (int)std::floor(pos.y);
            if (tx >= 0 && tx < w && ty >= 0 && ty < h) doorKeys.insert(ty * w + tx);
        }

        // If any perimeter tile matches a known door, region is guarded
        for (auto &pp : perimeter) {
            int key = pp.second * w + pp.first;
            if (doorKeys.find(key) != doorKeys.end()) return true;
        }

        return false;
    }

    // Check if this is a good corridor surveillance point
    // Prioritize: junctions, turns, long straight corridors
    /*! \brief Compute a priority score for placing a camera on a corridor tile. */
    static int GetCorridorPriority(Engine::TileMap* map, int x, int y, int w, int h)
    {
        if (!IsFloor(map, x, y)) return 0;
        if (IsLikelyRoom(map, x, y, w, h)) return 0; // Skip rooms entirely
        
        int open = OpenNeighborCount(map, x, y);
        const auto& cfg = GetCCTVPlacementConfig();
        
        // Junction (3-4 way) = highest priority
        if (open >= 3) return cfg.junctionPriority;
        
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
                return cfg.turnPriority;
            
            // Straight corridor (lower priority)
            return cfg.straightPriority;
        }
        
        // Dead end = low priority (but still place some)
        if (open == 1) return cfg.deadEndPriority;
        
        return 0;
    }

    /*! \brief Convert grid direction to rotation degrees. */
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
    /*! \brief Choose a facing direction for camera based on corridor geometry. */
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
    /*! \brief Place camera slightly toward adjacent wall inside floor tile. */
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
                const float margin = GetCCTVPlacementConfig().placementClampMargin;
                outPos.x = (std::max)(margin, (std::min)(outPos.x, (float)w - margin));
                outPos.y = (std::max)(margin, (std::min)(outPos.y, (float)h - margin));
                return true;
            }
        }

        // no adjacent wall found, just place at center
        outPos = Engine::Vector3(fx, fy, z);
        return true;
    }

    /*! \brief Generate CCTVInfo placements across zones using heuristics. */
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

        const auto& cfg = GetCCTVPlacementConfig();

        // Define spawn exclusion zone (top-left corner area where player spawns)
        const int spawnExcludeLeft = cfg.spawnExcludeLeft;
        const int spawnExcludeRight = cfg.spawnExcludeRight;
        const int spawnExcludeBottom = cfg.spawnExcludeBottom;
        const int spawnExcludeTop = cfg.spawnExcludeTop;

        //std::cout << "[CCTV Placement] Spawn exclusion zone: (" << spawnExcludeLeft << "," << spawnExcludeBottom 
        //         << ") to (" << spawnExcludeRight << "," << spawnExcludeTop << ") - Player spawns at (2, 2)\n";

        // Lambda to check if position is in spawn exclusion zone
        auto IsInSpawnZone = [&](int x, int y) -> bool {
            return (x >= spawnExcludeLeft && x <= spawnExcludeRight &&
                    y >= spawnExcludeBottom && y <= spawnExcludeTop);
        };

        // Collect ALL corridor candidates with priority scores per zone
        struct Candidate { int x, y, priority; };
        std::vector<Candidate> candidates[5]; // 1..4
        
        // Scan entire map for corridor tiles (not just stepped)
        // Use sampling 'step' to reduce CPU cost on large maps. Default step=1 scans every tile.
        int stepSize = (step > 1) ? step : 1;
        for (int y = 1; y < h - 1; y += stepSize)
        {
            for (int x = 1; x < w - 1; x += stepSize)
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

        //std::cout << "[CCTV Placement] Corridor candidates found per zone (excluding spawn area):\n";
        for (int zid = 1; zid <= 4; ++zid)
        {
            //std::cout << "  Zone " << zid << ": " << candidates[zid].size() << " candidates\n";
        }
        // For each zone, pick up to `perZone` corridor CCTV positions, distributed and spaced
        for (int zid = 1; zid <= 4; ++zid) {
            // build filtered candidate list
            std::vector<Candidate> flat;
            for (auto &c : candidates[zid]) {
                int x = c.x, y = c.y;
                if (!IsCorridorTile(map, x, y)) continue;
                if (IsLikelyRoom(map, x, y, w, h)) continue;
                if (IsEnclosedByWallsAndMarkers(map, x, y, w, h)) continue;
                if (RegionGuardedByDoor(map, x, y, w, h)) continue;
                if (IsInSpawnZone(x, y)) continue;
                flat.push_back(c);
            }

            if (flat.empty()) {
                //std::cout << "[CCTV Placement] Zone " << zid << ": no corridor candidates after filtering\n";
                continue;
            }

            // precompute positions
            std::vector<Engine::Vector3> pos(flat.size());
            for (size_t i=0;i<flat.size();++i) pos[i] = Engine::Vector3((float)flat[i].x + 0.5f, (float)flat[i].y + 0.5f, z);

            // sort indices by priority desc for deterministic seed
            std::vector<int> idxs((int)flat.size()); for (int i=0;i<(int)flat.size();++i) idxs[i]=i;
            std::sort(idxs.begin(), idxs.end(), [&](int a,int b){ return flat[a].priority > flat[b].priority; });

            std::vector<int> selected;
            // seed with highest priority
            selected.push_back(idxs[0]);

            // farthest-first selection until we have perZone or run out
            while ((int)selected.size() < perZone && (int)selected.size() < (int)flat.size()) {
                int best = -1; float bestMinD = -1.0f; int bestPr = -1;
                for (int i=0;i<(int)flat.size();++i) {
                    bool used = false; for (int s : selected) if (s==i) { used=true; break; }
                    if (used) continue;
                    float minD = 1e30f;
                    for (int s : selected) {
                        float d2 = DistSq2D(pos[i], pos[s]); if (d2 < minD) minD = d2;
                    }
                    int pr = flat[i].priority;
                    if (minD > bestMinD || (fabs(minD - bestMinD) < 1e-6 && pr > bestPr)) { bestMinD = minD; best = i; bestPr = pr; }
                }
                if (best < 0) break;
                selected.push_back(best);
            }

            int placedZone = 0;
            // push selected into cams with final checks
            for (int s : selected) {
                if (placedZone >= perZone) break;
                int tx = flat[s].x, ty = flat[s].y;
                Engine::Vector3 ppos;
                if (!PlaceOnWallInsideFloor(map, w, h, tx, ty, z, eps, ppos)) continue;
                // check spacing vs existing cams in same zone
                bool tooClose = false;
                for (auto &e : cams) if (e.zoneId==zid && DistSq2D(e.pos, ppos) < minDist*minDist) { tooClose=true; break; }
                if (tooClose) continue;
                CCTVInfo info{}; info.zoneId = zid; info.tileX = tx; info.tileY = ty; info.pos = ppos;
                int dx=1,dy=0; ChooseFacingDir(map, tx, ty, dx, dy); info.rot = RotFromDir(dx,dy);
                cams.push_back(info); placedZone++;
            }

            //std::cout << "[CCTV Placement] Zone " << zid << ": Placed " << placedZone << "/" << perZone << " cameras\n";
        }

        //std::cout << "[CCTV Placement] Total cameras placed: " << cams.size() << "\n";
        return cams;
    }

} // namespace Game