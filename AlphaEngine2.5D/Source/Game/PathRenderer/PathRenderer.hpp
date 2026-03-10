/**
 * @file PathRenderer.hpp
 * @brief Renders a path on the floor to guide the player to a destination
 */
#pragma once

#include "Engine/Graphics/IRenderer.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Graphics/Color.hpp"
#include <vector>

namespace Engine {
    class TileMap;
}

namespace Game {

    /**
     * @class PathRenderer
     * @brief Renders visual waypoint markers on the floor showing path to destination
     * 
     * Uses A* pathfinding to compute optimal route, then renders glowing markers
     * on the floor tiles to guide the player.
     */
    class PathRenderer : public Engine::IRenderer {
    public:
        /** Path waypoints (tile coordinates) */
        std::vector<Engine::Vector2> pathWaypoints;
        
        /** Marker color (can be set per-card) */
        Engine::Color markerColor = Engine::Color(1.0f, 1.0f, 0.0f, 0.8f); // Yellow default
        
        /** Marker size */
        float markerSize = 0.3f;
        
        /** Marker height above floor */
        float markerHeight = 0.02f;
        
        /** Card ID this path is for (0-3, -1 for none) */
        int cardId = -1;
        
        /** Glow animation */
        float glowSpeed = 2.0f;
        float glowIntensity = 0.3f;
        
        /** Auto-fade when player gets close to destination */
        bool autoFade = true;
        float fadeDistance = 2.0f;
        float currentAlpha = 1.0f;
        
        /** Destination position */
        Engine::Vector2 destination;
        
        /** TileMap reference for pathfinding */
        Engine::TileMap* tileMap = nullptr;
        
        /** Player transform for distance checks */
        Engine::Transform* playerTransform = nullptr;
        
        // Lifecycle
        void Awake() override;
        void Update() override;
        void Render() override; // IRenderer interface - called during render phase
        
        /** Compute and set path from start to destination using A*, with card-specific color */
        void SetPath(const Engine::Vector2& start, const Engine::Vector2& dest, int cardID = -1, const Engine::Color& color = Engine::Color(1.0f, 1.0f, 0.0f, 0.8f));
        
        /** Clear the path */
        void ClearPath();
        
        /** Check if path is active */
        bool HasPath() const { return !pathWaypoints.empty(); }
        
    private:
        float animTime = 0.0f;
        
        /** A* pathfinding implementation */
        std::vector<Engine::Vector2> FindPath(const Engine::Vector2& start, const Engine::Vector2& goal);
        
        /** Render a single waypoint marker */
        void RenderMarker(const Engine::Vector2& position, float alpha, float glowMod);
    };

} // namespace Game
