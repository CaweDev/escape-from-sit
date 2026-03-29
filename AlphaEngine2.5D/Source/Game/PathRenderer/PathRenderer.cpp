/**
 * @file PathRenderer.cpp
 * @brief Implementation of PathRenderer component
 */
#include "PathRenderer.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Math/Mathf.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include "AEEngine.h"
#include <queue>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace
{
    struct PathRendererConfig : public Engine::IConfig
    {
        bool renderEnabled = false;
        bool verboseLogging = true;
        float clearAlphaThreshold = 0.1f;
        float renderDistanceLimit = 20.0f;
        int offscreenPaddingPercent = 120;
        float minPerpDist = 0.1f;
        float minScreenSize = 2.0f;
        float maxScreenSize = 100.0f;

        void FromJson(const Json::Value& j) override
        {
            renderEnabled = j.get("renderEnabled", renderEnabled).asBool();
            verboseLogging = j.get("verboseLogging", verboseLogging).asBool();
            clearAlphaThreshold = j.get("clearAlphaThreshold", clearAlphaThreshold).asFloat();
            renderDistanceLimit = j.get("renderDistanceLimit", renderDistanceLimit).asFloat();
            offscreenPaddingPercent = j.get("offscreenPaddingPercent", offscreenPaddingPercent).asInt();
            minPerpDist = j.get("minPerpDist", minPerpDist).asFloat();
            minScreenSize = j.get("minScreenSize", minScreenSize).asFloat();
            maxScreenSize = j.get("maxScreenSize", maxScreenSize).asFloat();
        }
    };

    PathRendererConfig g_pathRendererConfig;
    bool g_pathRendererConfigLoaded = false;

    const PathRendererConfig& GetPathRendererConfig()
    {
        if (!g_pathRendererConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/path_renderer.json", g_pathRendererConfig);
            if (g_pathRendererConfig.renderDistanceLimit < 0.1f) g_pathRendererConfig.renderDistanceLimit = 20.0f;
            if (g_pathRendererConfig.offscreenPaddingPercent < 100) g_pathRendererConfig.offscreenPaddingPercent = 100;
            if (g_pathRendererConfig.minPerpDist < 0.001f) g_pathRendererConfig.minPerpDist = 0.001f;
            if (g_pathRendererConfig.minScreenSize < 0.1f) g_pathRendererConfig.minScreenSize = 0.1f;
            if (g_pathRendererConfig.maxScreenSize < g_pathRendererConfig.minScreenSize) g_pathRendererConfig.maxScreenSize = g_pathRendererConfig.minScreenSize;
            g_pathRendererConfigLoaded = true;
        }
        return g_pathRendererConfig;
    }
}

namespace Game {

/*!
 * \brief [AUTO] Awake
 */
void PathRenderer::Awake() {
    // Component will be initialized when SetPath is called
    /* std::cout << "[PathRenderer] Awake() called - Component created successfully!" << std::endl;
    std::cout << "[PathRenderer] GameObject: " << (gameObject ? gameObject->name.c_str() : "NULL") << std::endl;
    std::cout << "[PathRenderer] Enabled: " << (enabled ? "YES" : "NO") << std::endl;*/
}

/*!
 * \brief [AUTO] Update
 */
void PathRenderer::Update() {
    if (!HasPath()) return;
    
    // Update animation time
    animTime += Engine::Time::deltaTime * glowSpeed;
    
    // Auto-fade when close to destination
    if (autoFade && playerTransform) {
        Engine::Vector2 playerPos(playerTransform->position.x, playerTransform->position.y);
        float distToDest = std::sqrt(
            (destination.x - playerPos.x) * (destination.x - playerPos.x) +
            (destination.y - playerPos.y) * (destination.y - playerPos.y)
        );
        
        if (distToDest < fadeDistance) {
            // Fade out as we approach
            currentAlpha = distToDest / fadeDistance;
            if (currentAlpha < GetPathRendererConfig().clearAlphaThreshold) {
                ClearPath(); // Remove path when very close
            }
        } else {
            currentAlpha = 1.0f;
        }
    }
}

/*!
 * \brief [AUTO] Render
 */
void PathRenderer::Render() {
    // DISABLED by default: 3D path markers can be enabled via config
    if (!GetPathRendererConfig().renderEnabled) return;
    
    static int renderCallCount = 0;
    
    if (!HasPath()) {
        if (GetPathRendererConfig().verboseLogging && renderCallCount % 60 == 0) { // Log once per second at 60fps
            //std::cout << "[PathRenderer::Render] No path (waypoints empty)" << std::endl;
        }
        renderCallCount++;
        return;
    }
    
    if (currentAlpha <= 0.0f) {
        if (GetPathRendererConfig().verboseLogging && renderCallCount % 60 == 0) {
            //std::cout << "[PathRenderer::Render] Path exists but alpha=0" << std::endl;
        }
        renderCallCount++;
        return;
    }
    
    if (!Engine::Camera::main) {
        if (GetPathRendererConfig().verboseLogging && renderCallCount % 60 == 0) {
            //std::cout << "[PathRenderer::Render] No Camera::main!" << std::endl;
        }
        renderCallCount++;
        return;
    }
    
    // Only log first few renders to avoid spam
    if (GetPathRendererConfig().verboseLogging && renderCallCount < 5) {
        /*std::cout << "[PathRenderer::Render] Rendering " << pathWaypoints.size() 
                  << " markers with alpha=" << currentAlpha << std::endl;*/
    }
    renderCallCount++;
    
    // Render each waypoint marker
    for (size_t i = 0; i < pathWaypoints.size(); ++i) {
        const auto& wp = pathWaypoints[i];
        
        // Slightly offset each marker's animation phase for wave effect
        float phaseOffset = (float)i * 0.2f;
        float localGlow = 1.0f + std::sin(animTime + phaseOffset) * glowIntensity;
        
        RenderMarker(wp, currentAlpha, localGlow);
    }
}

/*!
 * \brief [AUTO] SetPath
 * \param start [in]
 * \param dest [in]
 * \param cardID [in]
 * \param color [in]
 */
void PathRenderer::SetPath(const Engine::Vector2& start, const Engine::Vector2& dest, int cardID, const Engine::Color& color) {
    destination = dest;
    cardId = cardID;
    markerColor = color;
    pathWaypoints = FindPath(start, dest);
    
    //if (pathWaypoints.empty()) {
    //    //std::cout << "[PathRenderer] No path found from (" << start.x << ", " << start.y 
    //    //          << ") to (" << dest.x << ", " << dest.y << ")\n";
    //} else {
    //    //const char* cardNames[] = {"Green", "Purple", "Red", "Yellow"};
    //    //std::cout << "[PathRenderer] ===== PATH SET SUCCESSFULLY =====" << std::endl;
    //    //std::cout << "[PathRenderer] Card: " << (cardID >= 0 && cardID <= 3 ? cardNames[cardID] : "Unknown") << " (ID=" << cardID << ")" << std::endl;
    //    //std::cout << "[PathRenderer] Path computed with " << pathWaypoints.size() << " waypoints\n";
    //    //std::cout << "[PathRenderer] Start: (" << start.x << ", " << start.y << ")" << std::endl;
    //    //std::cout << "[PathRenderer] Dest: (" << dest.x << ", " << dest.y << ")" << std::endl;
    //    //std::cout << "[PathRenderer] Color: (" << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
    //    //std::cout << "[PathRenderer] Enabled: " << (enabled ? "YES" : "NO") << std::endl;
    //    //std::cout << "[PathRenderer] Component ready to render!" << std::endl;
    //}
}

/*!
 * \brief [AUTO] ClearPath
 */
void PathRenderer::ClearPath() {
    pathWaypoints.clear();
    currentAlpha = 1.0f;
    cardId = -1;
    //std::cout << "[PathRenderer] Path cleared" << std::endl;
}

/*!
 * \brief [AUTO] RenderMarker
 * \param position [in]
 * \param alpha [in]
 * \param glowMod [in]
 */
void PathRenderer::RenderMarker(const Engine::Vector2& position, float alpha, float glowMod) {
    if (!Engine::Camera::main || !Engine::Camera::main->transform) return;
    
    // World position of marker (center of tile, just above floor)
    float worldX = position.x + 0.5f;
    float worldY = position.y + 0.5f;
    float worldZ = markerHeight;
    
    // Get camera position and rotation
    Engine::Vector3 camPos = Engine::Camera::main->transform->position;
    float camAngle = Engine::Camera::main->transform->rotation * Engine::Mathf::Deg2Rad;
    
    // Calculate distance to marker
    float dx = worldX - camPos.x;
    float dy = worldY - camPos.y;
    float distance = std::sqrt(dx * dx + dy * dy);
    
    // Only render if reasonably close (optimization)
    if (distance > GetPathRendererConfig().renderDistanceLimit) return;
    
    // Calculate if marker is in front of camera
    float dirX = std::cos(camAngle);
    float dirY = std::sin(camAngle);
    float dot = dx * dirX + dy * dirY;
    if (dot <= 0) return; // Behind camera
    
    // Get screen dimensions
    int screenW = AEGfxGetWindowWidth();
    int screenH = AEGfxGetWindowHeight();
    
    // Calculate perpendicular distance (correct raycasting distance)
    float perpDist = std::abs(dx * dirX + dy * dirY);
    if (perpDist < GetPathRendererConfig().minPerpDist) perpDist = GetPathRendererConfig().minPerpDist;
    
    // Calculate screen position
    float fov = Engine::Camera::main->fov * Engine::Mathf::Deg2Rad;
    float planeLength = std::tan(fov / 2.0f);
    
    // Calculate angle to marker relative to camera
    float angleToMarker = std::atan2(dy, dx);
    float relativeAngle = angleToMarker - camAngle;
    
    // Normalize angle to [-PI, PI]
    const float MY_PI = 3.14159265f;
    while (relativeAngle > MY_PI) relativeAngle -= 2.0f * MY_PI;
    while (relativeAngle < -MY_PI) relativeAngle += 2.0f * MY_PI;
    
    // Screen X position based on angle
    float screenXNorm = relativeAngle / (fov / 2.0f); // -1 to 1
    const float offscreenPadding = (float)GetPathRendererConfig().offscreenPaddingPercent / 100.0f;
    if (std::abs(screenXNorm) > offscreenPadding) return; // Off screen horizontally
    float screenX = screenW / 2.0f + screenXNorm * (screenW / 2.0f);
    
    // Screen Y position (floor markers)
    float markerScreenHeight = (screenW / (perpDist * planeLength * 2.0f));
    float heightOffset = (camPos.z - worldZ) * markerScreenHeight;
    int horizon = screenH / 2 + (int)Engine::Camera::main->pitch;
    float screenY = horizon + heightOffset;
    
    // Screen size for marker
    float screenSize = (markerSize * screenW) / (perpDist * planeLength * 2.0f);
    if (screenSize < GetPathRendererConfig().minScreenSize) screenSize = GetPathRendererConfig().minScreenSize;
    if (screenSize > GetPathRendererConfig().maxScreenSize) screenSize = GetPathRendererConfig().maxScreenSize;
    
    // Skip if completely off-screen
    if (screenX + screenSize < 0 || screenX - screenSize > screenW) return;
    if (screenY + screenSize < 0 || screenY - screenSize > screenH) return;
    
    // Apply fog if enabled
    float fogFactor = 1.0f;
    if (Engine::Camera::main->fogEnabled) {
        fogFactor = std::exp(-perpDist / Engine::Camera::main->fogDistance);
        if (fogFactor < Engine::Camera::main->fogMinVisibility) 
            fogFactor = Engine::Camera::main->fogMinVisibility;
        if (fogFactor > Engine::Camera::main->fogMaxVisibility) 
            fogFactor = Engine::Camera::main->fogMaxVisibility;
    }
    
    // Final color with glow, alpha, and fog
    float finalR = markerColor.r * glowMod * fogFactor;
    float finalG = markerColor.g * glowMod * fogFactor;
    float finalB = markerColor.b * glowMod * fogFactor;
    float finalA = markerColor.a * alpha * fogFactor;
    
    // Clamp colors
    if (finalR > 1.0f) finalR = 1.0f;
    if (finalG > 1.0f) finalG = 1.0f;
    if (finalB > 1.0f) finalB = 1.0f;
    if (finalA < 0.05f) return; // Too faint to see
    
    // Convert to AE screen space (origin at center)
    float aeX = screenX - screenW / 2.0f;
    float aeY = screenH / 2.0f - screenY;
    
    // Set up rendering
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(finalA);
    AEGfxSetColorToMultiply(finalR, finalG, finalB, 1.0f);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    
    // Create transform matrix
    AEMtx33 scale, trans, worldMtx;
    AEMtx33Scale(&scale, screenSize, screenSize);
    AEMtx33Trans(&trans, aeX, aeY);
    AEMtx33Concat(&worldMtx, &trans, &scale);
    AEGfxSetTransform(worldMtx.m);
    
    // Create and draw a simple quad for this marker
    AEGfxMeshStart();
    
    // Pack color into vertex color (ARGB format)
    u32 vertexColor = 0xFFFFFFFF; // White - color handled by SetColorToMultiply
    
    // Two triangles to form a square
    AEGfxTriAdd(
        -0.5f, -0.5f, vertexColor, 0, 0,
        0.5f, -0.5f, vertexColor, 1, 0,
        -0.5f, 0.5f, vertexColor, 0, 1);
    AEGfxTriAdd(
        0.5f, -0.5f, vertexColor, 1, 0,
        0.5f, 0.5f, vertexColor, 1, 1,
        -0.5f, 0.5f, vertexColor, 0, 1);
    
    AEGfxVertexList* mesh = AEGfxMeshEnd();
    
    if (mesh) {
        AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
        AEGfxMeshFree(mesh);
    }
}

// A* pathfinding implementation
/*!
 * \brief [AUTO] FindPath
 * \param start [in]
 * \param goal [in]
 * \return 
 */
std::vector<Engine::Vector2> PathRenderer::FindPath(const Engine::Vector2& start, const Engine::Vector2& goal) {
    std::vector<Engine::Vector2> path;
    
    if (!tileMap) {
        //std::cout << "[PathRenderer] No TileMap reference for pathfinding\n";
        return path;
    }
    
    // Convert to tile coordinates
    int startX = (int)std::floor(start.x);
    int startY = (int)std::floor(start.y);
    int goalX = (int)std::floor(goal.x);
    int goalY = (int)std::floor(goal.y);
    
    // Check if start and goal are walkable
    if (tileMap->IsWall(startX, startY) || tileMap->IsWall(goalX, goalY)) {
        //std::cout << "[PathRenderer] Start or goal is blocked\n";
        return path;
    }
    
    // A* data structures
    struct Node {
        int x, y;
        float g, h, f;
        int parentX, parentY;
        
        bool operator>(const Node& other) const { return f > other.f; }
    };
    
    auto hash = [](int x, int y) { return y * 10000 + x; };
    auto heuristic = [](int x1, int y1, int x2, int y2) -> float {
        return std::sqrt((float)((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)));
    };
    
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openList;
    std::unordered_map<int, Node> allNodes;
    std::unordered_map<int, bool> closedSet;
    
    // Add start node
    Node startNode = {startX, startY, 0.0f, heuristic(startX, startY, goalX, goalY), 0.0f, -1, -1};
    startNode.f = startNode.g + startNode.h;
    openList.push(startNode);
    allNodes[hash(startX, startY)] = startNode;
    
    // 8-directional movement
    const int dx[] = {-1, 0, 1, 0, -1, 1, 1, -1};
    const int dy[] = {0, 1, 0, -1, 1, 1, -1, -1};
    const float costs[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.414f, 1.414f, 1.414f, 1.414f}; // Diagonal = sqrt(2)
    
    bool foundPath = false;
    int iterations = 0;
    const int maxIterations = 10000; // Prevent infinite loops
    
    while (!openList.empty() && iterations < maxIterations) {
        iterations++;
        
        Node current = openList.top();
        openList.pop();
        
        int currentHash = hash(current.x, current.y);
        
        // Skip if already processed
        if (closedSet[currentHash]) continue;
        closedSet[currentHash] = true;
        
        // Check if reached goal
        if (current.x == goalX && current.y == goalY) {
            foundPath = true;
            
            // Reconstruct path
            std::vector<Engine::Vector2> reversePath;
            int nx = current.x;
            int ny = current.y;
            
            while (nx != -1 && ny != -1) {
                reversePath.push_back(Engine::Vector2((float)nx, (float)ny));
                int nodeHash = hash(nx, ny);
                if (allNodes.find(nodeHash) == allNodes.end()) break;
                Node& n = allNodes[nodeHash];
                nx = n.parentX;
                ny = n.parentY;
            }
            
            // Reverse to get path from start to goal
            path.assign(reversePath.rbegin(), reversePath.rend());
            
            // Simplify path - only keep waypoints at turns (optional, reduces clutter)
            std::vector<Engine::Vector2> simplifiedPath;
            if (path.size() > 0) {
                simplifiedPath.push_back(path[0]);
                
                for (size_t i = 1; i < path.size() - 1; ++i) {
                    Engine::Vector2 prev = path[i - 1];
                    Engine::Vector2 curr = path[i];
                    Engine::Vector2 next = path[i + 1];
                    
                    // Check if direction changed
                    Engine::Vector2 dir1(curr.x - prev.x, curr.y - prev.y);
                    Engine::Vector2 dir2(next.x - curr.x, next.y - curr.y);
            
                    if (std::abs(dir1.x - dir2.x) > 0.01f || std::abs(dir1.y - dir2.y) > 0.01f) {
                        simplifiedPath.push_back(curr);
                    }
                }
                
                if (path.size() > 1) {
                    simplifiedPath.push_back(path[path.size() - 1]);
                }
            }
            
            path = simplifiedPath;
            break;
        }
        
        // Explore neighbors
        for (int i = 0; i < 8; ++i) {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];
            
            // Check bounds and walkability
            if (nx < 0 || ny < 0 || nx >= tileMap->width || ny >= tileMap->height) continue;
            if (tileMap->IsWall(nx, ny)) continue;
            
            // For diagonal movement, check if both adjacent tiles are walkable (no corner cutting)
            if (i >= 4) {
                if (tileMap->IsWall(current.x + dx[i], current.y) || 
                    tileMap->IsWall(current.x, current.y + dy[i])) {
                    continue;
                }
            }
            
            int neighborHash = hash(nx, ny);
            if (closedSet[neighborHash]) continue;
            
            float tentativeG = current.g + costs[i];
            
            // Check if this path to neighbor is better
            bool isBetter = false;
            if (allNodes.find(neighborHash) == allNodes.end()) {
                isBetter = true;
            } else if (tentativeG < allNodes[neighborHash].g) {
                isBetter = true;
            }
            
            if (isBetter) {
                Node neighbor;
                neighbor.x = nx;
                neighbor.y = ny;
                neighbor.g = tentativeG;
                neighbor.h = heuristic(nx, ny, goalX, goalY);
                neighbor.f = neighbor.g + neighbor.h;
                neighbor.parentX = current.x;
                neighbor.parentY = current.y;
                
                allNodes[neighborHash] = neighbor;
                openList.push(neighbor);
            }
        }
    }
    
  /*  if (!foundPath) {
        if (GetPathRendererConfig().verboseLogging)
            std::cout << "[PathRenderer] No path found after " << iterations << " iterations\n";
    }*/
    
    return path;
}

} // namespace Game
