// /*!
//  * \file AlphaEngine2.5D\Source\Game\LevelEditor\LevelEditor.hpp
//  * \author DigiPen Student
//  * \date 2026-03-29
//  * \brief [AUTO] Added Doxygen file header
//  */

// #pragma once

// #include "Engine/Scene/Scene.hpp"
// #include "Engine/TileMap/TileMap.hpp"
// #include "Engine/Input/Input.hpp"
// #include "Engine/Input/KeyCode.hpp"
// #include "Engine/UI/UI.hpp"
// #include "Engine/Graphics/Camera.hpp"
// #include "Engine/Core/Transform.hpp"
// #include "AEEngine.h"
// #include <vector>
// #include <string>
// #include <functional>

// namespace Game {

// using namespace Engine;

// /**
//  * enum EditorMode
//  * @brief What to place on tiles
//  */
// enum class EditorMode {
//     Wall,           // Build walls
//     Door,           // Convert floor to door
//     Terminal,       // Mark terminal location
//     Exit,           // Mark exit location
//     ControlRoom,    // Mark control room door
//     GreenCard,      // Mark green access card spawn
//     PurpleCard,     // Mark purple access card spawn
//     RedCard,        // Mark red access card spawn
//     YellowCard,     // Mark yellow access card spawn
//     Erase           // Convert back to floor
// };

// /**
//  * @class LevelEditor
//  * @brief Visual top-down map editor - click on floor tiles to place objects
//  */
// class LevelEditor {
// public:
//     LevelEditor();
//     ~LevelEditor();

//     // Core functions
//     void Initialize(Scene* scene, TileMap* map, Camera* camera, Transform* playerTransform);
//     void Update();
//     void Render();
//     void ToggleEditor();
    
//     bool IsActive() const { return isActive; }

// private:
//     void CreateUI();
//     void UpdateUI();
//     void HandleInput();
//     void RenderMap();
//     void RenderMapGrid();
    
//     void SetMode(EditorMode mode);
//     void HandleMapClick();
//     void SaveMapTxt(const std::string& filename);
//     void SpawnDoorAt(int x, int y);       // Real-time door spawning
//     void SpawnTerminalAt(int x, int y);   // Real-time terminal spawning
    
//     Vector2 ScreenToMapCoords(float screenX, float screenY);
//     bool IsFloorTile(int x, int y);

//     bool isActive = false;
//     Scene* scene = nullptr;
//     TileMap* tileMap = nullptr;
//     Camera* camera = nullptr;
//     Transform* playerTransform = nullptr;

//     EditorMode currentMode = EditorMode::Door;
    
//     // Map view state
//     int mapWidth = 0;
//     int mapHeight = 0;
//     float viewOffsetX = 0.0f;
//     float viewOffsetY = 0.0f;
//     float viewZoom = 1.0f;
//     float tileSize = 8.0f; // Pixels per tile at zoom=1
    
//     // Mouse state
//     bool isDragging = false;
//     float lastMouseX = 0.0f;
//     float lastMouseY = 0.0f;
//     int hoveredTileX = -1;
//     int hoveredTileY = -1;

//     // UI
//     UI::Canvas* editorCanvas = nullptr;
//     UI::Panel* toolPanel = nullptr;
//     UI::Panel* mapViewPanel = nullptr;
//     UI::Text* statusText = nullptr;
//     UI::Text* helpText = nullptr;
//     UI::Button* wallButton = nullptr;
//     UI::Button* doorButton = nullptr;
//     UI::Button* terminalButton = nullptr;
//     UI::Button* exitButton = nullptr;
//     UI::Button* controlRoomButton = nullptr;
//     UI::Button* greenCardButton = nullptr;
//     UI::Button* purpleCardButton = nullptr;
//     UI::Button* redCardButton = nullptr;
//     UI::Button* yellowCardButton = nullptr;
//     UI::Button* eraseButton = nullptr;
//     UI::Button* saveButton = nullptr;
//     UI::Button* closeButton = nullptr;
    
//     // Meshes for rendering
//     AEGfxVertexList* tileMesh = nullptr;
// };

// } // namespace Game
