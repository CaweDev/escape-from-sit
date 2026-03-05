#include "LevelEditor.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Core/AssetManager.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Graphics/TileMapRenderer.hpp"
#include "Engine/TileMap/Door.hpp"
#include "Engine/Graphics/DoorRenderer.hpp"
#include "Engine/Physics/BoxCollider2D.hpp"
#include "Engine/Interaction/Interactable.hpp"
#include "Game/CCTV/CCTVTerminal.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>

#undef min
#undef max

namespace Game {

LevelEditor::LevelEditor() {}

LevelEditor::~LevelEditor() {
    if (tileMesh) {
        AEGfxMeshFree(tileMesh);
    }
    if (editorCanvas && UI::UIManager::GetInstance()) {
        // Cleanup handled by UIManager
    }
}

void LevelEditor::Initialize(Scene* scn, TileMap* map, Camera* cam, Transform* playerTf) {
    this->scene = scn;
    this->tileMap = map;
    this->camera = cam;
    this->playerTransform = playerTf;
    
    if (tileMap) {
        mapWidth = tileMap->width;
        mapHeight = tileMap->height;
    }
    
    CreateUI();
    
    // Create tile mesh (unit square)
    AEGfxMeshStart();
    AEGfxTriAdd(
        0.0f, 0.0f, 0xFFFFFFFF, 0.0f, 0.0f,
        1.0f, 0.0f, 0xFFFFFFFF, 1.0f, 0.0f,
        0.0f, 1.0f, 0xFFFFFFFF, 0.0f, 1.0f);
    AEGfxTriAdd(
        1.0f, 0.0f, 0xFFFFFFFF, 1.0f, 0.0f,
        1.0f, 1.0f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.0f, 1.0f, 0xFFFFFFFF, 0.0f, 1.0f);
    tileMesh = AEGfxMeshEnd();
}

void LevelEditor::CreateUI() {
    if (!UI::UIManager::GetInstance()) return;

    editorCanvas = UI::UIManager::GetInstance()->CreateCanvas();
    editorCanvas->SetSortOrder(999);
    editorCanvas->SetReferenceResolution(1280.0f, 720.0f);
    editorCanvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);

    // Map view panel (centered, takes most of screen)
    mapViewPanel = editorCanvas->CreateChild<UI::Panel>();
    mapViewPanel->name = "MapViewPanel";
    mapViewPanel->SetAnchors({0.0f, 0.0f}, {1.0f, 1.0f});
    mapViewPanel->SetSizeDelta({-20.0f, -20.0f});
    mapViewPanel->SetBackgroundColor(Engine::Color(0.15f, 0.15f, 0.15f, 0.95f)); // Brighter background
    mapViewPanel->visible = false;

    // Tool panel (top bar)
    toolPanel = editorCanvas->CreateChild<UI::Panel>();
    toolPanel->name = "ToolPanel";
    toolPanel->SetAnchors({0.0f, 1.0f}, {1.0f, 1.0f});
    toolPanel->SetPivot({0.5f, 1.0f});
    toolPanel->SetSizeDelta({0.0f, 60.0f});
    toolPanel->SetBackgroundColor(Engine::Color(0.15f, 0.15f, 0.15f, 1.0f));
    toolPanel->visible = false;

    // Title
    auto title = toolPanel->CreateChild<UI::Text>();
    title->SetAnchors({0.0f, 0.0f}, {0.0f, 1.0f});
    title->SetPivot({0.0f, 0.5f});
    title->SetAnchoredPosition({20.0f, 0.0f});
    title->SetSizeDelta({200.0f, 0.0f});
    title->SetText("MAP EDITOR");
    title->SetFontSize(2.0f);
    title->SetColor(Engine::Color::white());
    title->SetAnchor(UI::TextAnchor::MiddleLeft);

    // Horizontal layout for buttons
    auto buttonLayout = toolPanel->CreateChild<UI::HorizontalLayoutGroup>();
    buttonLayout->SetAnchors({0.2f, 0.0f}, {0.9f, 1.0f});
    buttonLayout->SetSizeDelta({0.0f, -10.0f});
    buttonLayout->SetSpacing(10.0f);
    buttonLayout->SetChildAlignment(UI::ChildAlignment::MiddleCenter);

    // Mode buttons
    wallButton = buttonLayout->CreateChild<UI::Button>();
    wallButton->SetSizeDelta({120.0f, 0.0f});
    wallButton->SetText("Wall (1)");
    wallButton->SetBackgroundColor(Engine::Color(0.5f, 0.5f, 0.5f, 1.0f));
    wallButton->onClick = [this]() { SetMode(EditorMode::Wall); };

    doorButton = buttonLayout->CreateChild<UI::Button>();
    doorButton->SetSizeDelta({120.0f, 0.0f});
    doorButton->SetText("Door (2)");
    doorButton->SetBackgroundColor(Engine::Color(0.2f, 0.5f, 0.8f, 1.0f));
    doorButton->onClick = [this]() { SetMode(EditorMode::Door); };

    terminalButton = buttonLayout->CreateChild<UI::Button>();
    terminalButton->SetSizeDelta({120.0f, 0.0f});
    terminalButton->SetText("Terminal (3)");
    terminalButton->SetBackgroundColor(Engine::Color(0.2f, 0.5f, 0.8f, 1.0f));
    terminalButton->onClick = [this]() { SetMode(EditorMode::Terminal); };

    exitButton = buttonLayout->CreateChild<UI::Button>();
    exitButton->SetSizeDelta({120.0f, 0.0f});
    exitButton->SetText("Exit (4)");
    exitButton->SetBackgroundColor(Engine::Color(0.2f, 0.5f, 0.8f, 1.0f));
    exitButton->onClick = [this]() { SetMode(EditorMode::Exit); };

    eraseButton = buttonLayout->CreateChild<UI::Button>();
    eraseButton->SetSizeDelta({120.0f, 0.0f});
    eraseButton->SetText("Delete (5)");
    eraseButton->SetBackgroundColor(Engine::Color(0.8f, 0.3f, 0.3f, 1.0f));
    eraseButton->onClick = [this]() { SetMode(EditorMode::Erase); };

    // Save button
    saveButton = buttonLayout->CreateChild<UI::Button>();
    saveButton->SetSizeDelta({120.0f, 0.0f});
    saveButton->SetText("Save (S)");
    saveButton->SetBackgroundColor(Engine::Color(0.3f, 0.7f, 0.3f, 1.0f));
    saveButton->onClick = [this]() { SaveMapTxt("Assets/map.txt"); };

    // Close button
    closeButton = buttonLayout->CreateChild<UI::Button>();
    closeButton->SetSizeDelta({120.0f, 0.0f});
    closeButton->SetText("Close (F1)");
    closeButton->SetBackgroundColor(Engine::Color(0.5f, 0.5f, 0.5f, 1.0f));
    closeButton->onClick = [this]() { ToggleEditor(); };

    // Status text (bottom)
    statusText = editorCanvas->CreateChild<UI::Text>();
    statusText->SetAnchors({0.0f, 0.0f}, {1.0f, 0.0f});
    statusText->SetPivot({0.5f, 0.0f});
    statusText->SetAnchoredPosition({0.0f, 10.0f});
    statusText->SetSizeDelta({0.0f, 30.0f});
    statusText->SetText("Mode: Door | Tile: (--, --) | Zoom: 100% | Click floor tiles to edit");
    statusText->SetFontSize(1.2f);
    statusText->SetColor(Engine::Color::white());
    statusText->SetAnchor(UI::TextAnchor::MiddleCenter);
    statusText->visible = false;

    // Help text
    helpText = editorCanvas->CreateChild<UI::Text>();
    helpText->SetAnchors({1.0f, 1.0f}, {1.0f, 1.0f});
    helpText->SetPivot({1.0f, 1.0f});
    helpText->SetAnchoredPosition({-10.0f, -70.0f});
    helpText->SetSizeDelta({300.0f, 150.0f});
    helpText->SetText(
        "CONTROLS:\n"
        "LMB: Place/Delete\n"
        "RMB: Pan (drag)\n"
        "Scroll: Zoom\n"
        "1-5: Select mode\n"
        "1: BUILD WALLS\n"
        "5: DELETE any tile\n"
        "S: Save map.txt\n"
        "F1: Close editor"
    );
    helpText->SetFontSize(1.0f);
    helpText->SetColor(Engine::Color(1.0f, 1.0f, 0.0f, 1.0f));
    helpText->SetAnchor(UI::TextAnchor::UpperRight);
    helpText->visible = false;
}

void LevelEditor::Update() {
    if (!isActive) return;
    
    HandleInput();
    UpdateUI();
}

void LevelEditor::HandleInput() {
// Mode selection
if (Input::GetKeyDown(KeyCode::Alpha1)) SetMode(EditorMode::Wall);
if (Input::GetKeyDown(KeyCode::Alpha2)) SetMode(EditorMode::Door);
if (Input::GetKeyDown(KeyCode::Alpha3)) SetMode(EditorMode::Terminal);
if (Input::GetKeyDown(KeyCode::Alpha4)) SetMode(EditorMode::Exit);
if (Input::GetKeyDown(KeyCode::Alpha5)) SetMode(EditorMode::Erase);

    // Save
    if (Input::GetKeyDown(KeyCode::S)) {
        SaveMapTxt("Assets/map.txt");
    }

    // Get mouse position
    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);
    
    // Pan with right mouse button
    if (Input::GetMouseButtonDown(1)) {
        isDragging = true;
        lastMouseX = (float)mouseX;
        lastMouseY = (float)mouseY;
    }
    
    if (Input::GetMouseButtonUp(1)) {
        isDragging = false;
    }
    
    if (isDragging && Input::GetMouseButton(1)) {
        float deltaX = (float)mouseX - lastMouseX;
        float deltaY = (float)mouseY - lastMouseY;
        viewOffsetX += deltaX;
        viewOffsetY -= deltaY; // Invert Y
        lastMouseX = (float)mouseX;
        lastMouseY = (float)mouseY;
    }

    // Zoom with mouse wheel (simulated with +/- keys for now)
    if (Input::GetKeyDown(KeyCode::Equals)) {
        viewZoom = std::min(viewZoom * 1.2f, 4.0f);
    }
    if (Input::GetKeyDown(KeyCode::Minus)) {
        viewZoom = std::max(viewZoom * 0.8f, 0.25f);
    }

    // Update hovered tile
    Vector2 mapCoords = ScreenToMapCoords((float)mouseX, (float)mouseY);
    hoveredTileX = (int)mapCoords.x;
    hoveredTileY = (int)mapCoords.y;

    // Click to edit
    if (Input::GetMouseButtonDown(0)) {
        HandleMapClick();
    }
}

void LevelEditor::HandleMapClick() {
    if (hoveredTileX < 0 || hoveredTileX >= mapWidth) return;
    if (hoveredTileY < 0 || hoveredTileY >= mapHeight) return;
    
    if (!tileMap) return;
    
    int currentTile = (int)tileMap->GetTile(hoveredTileX, hoveredTileY);
    
    // Erase mode - convert ANY tile back to floor (0)
    if (currentMode == EditorMode::Erase) {
        tileMap->SetTile(hoveredTileX, hoveredTileY, Engine::TileType::Empty);
        std::cout << "[LevelEditor] Erased tile at (" << hoveredTileX << ", " << hoveredTileY 
                  << ") - converted to floor (LIVE UPDATE)\n";
        return;
    }
    
    // Wall mode - convert ANY tile to wall (can overwrite anything)
    if (currentMode == EditorMode::Wall) {
        tileMap->SetTile(hoveredTileX, hoveredTileY, Engine::TileType::Wall);
        std::cout << "[LevelEditor] Built wall at (" << hoveredTileX << ", " << hoveredTileY 
                  << ") (LIVE UPDATE)\n";
        return;
    }
    
    // Other modes - only allow editing floor tiles (0)
    if (currentTile != 0) {
        std::cout << "[LevelEditor] Can only place on floor tiles (0). Tile at (" 
                  << hoveredTileX << ", " << hoveredTileY << ") is " << currentTile 
                  << ". Use Delete (5) to clear first, or Wall (1) to build.\n";
        return;
    }
    
    // Apply change
    Engine::TileType newTile = Engine::TileType::Empty;
    
    switch (currentMode) {
        case EditorMode::Door:
            newTile = Engine::TileType::Door; // Value 10
            std::cout << "[LevelEditor] Placed door at (" << hoveredTileX << ", " << hoveredTileY << ") (SPAWNING NOW)\n";
            // SPAWN DOOR IMMEDIATELY
            SpawnDoorAt(hoveredTileX, hoveredTileY);
            break;
        case EditorMode::Terminal:
            newTile = static_cast<Engine::TileType>(11); // Terminal marker
            std::cout << "[LevelEditor] Placed terminal at (" << hoveredTileX << ", " << hoveredTileY << ") (SPAWNING NOW)\n";
            // SPAWN TERMINAL IMMEDIATELY
            SpawnTerminalAt(hoveredTileX, hoveredTileY);
            break;
        case EditorMode::Exit:
            newTile = static_cast<Engine::TileType>(12); // Exit marker
            std::cout << "[LevelEditor] Placed exit at (" << hoveredTileX << ", " << hoveredTileY << ") (LIVE UPDATE)\n";
            break;
        case EditorMode::Wall:
        case EditorMode::Erase:
            // Already handled above
            break;
    }
    
    tileMap->SetTile(hoveredTileX, hoveredTileY, newTile);
    
    // Note: Changes are applied to TileMap immediately
    // Renderer and collider read from TileMap each frame automatically
    // Close editor (F1) to see changes in-game
}

Vector2 LevelEditor::ScreenToMapCoords(float screenX, float screenY) {
    int screenWidth = AEGfxGetWindowWidth();
    int screenHeight = AEGfxGetWindowHeight();
    
    // Convert screen to centered coords
    float centeredX = screenX - screenWidth / 2.0f;
    float centeredY = screenHeight / 2.0f - screenY;
    
    // Apply view offset
    centeredX -= viewOffsetX;
    centeredY -= viewOffsetY;
    
    // Apply zoom
    float scaledTileSize = tileSize * viewZoom;
    
    // Map origin offset (standard orientation)
    float mapOriginX = -mapWidth * scaledTileSize / 2.0f;
    float mapOriginY = -mapHeight * scaledTileSize / 2.0f;
    
    // Convert to map coordinates (standard mapping)
    float mapX = (centeredX - mapOriginX) / scaledTileSize;
    float mapY = (centeredY - mapOriginY) / scaledTileSize;
    
    return Vector2(mapX, mapY);
}

bool LevelEditor::IsFloorTile(int x, int y) {
    if (!tileMap) return false;
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) return false;
    return (int)tileMap->GetTile(x, y) == 0;
}

void LevelEditor::SetMode(EditorMode mode) {
    currentMode = mode;
    const char* modeNames[] = {"Wall", "Door", "Terminal", "Exit", "Delete"};
    std::cout << "[LevelEditor] Mode: " << modeNames[(int)mode] << "\n";
}

void LevelEditor::UpdateUI() {
    if (!statusText) return;

    const char* modeNames[] = {"Wall", "Door", "Terminal", "Exit", "Delete"};
    int zoomPercent = (int)(viewZoom * 100.0f);
    
    // Show actual tile value for debugging
    int currentTileValue = -999;
    if (tileMap && hoveredTileX >= 0 && hoveredTileX < mapWidth && hoveredTileY >= 0 && hoveredTileY < mapHeight) {
        currentTileValue = (int)tileMap->GetTile(hoveredTileX, hoveredTileY);
    }
    
    std::ostringstream oss;
    oss << "Mode: " << modeNames[(int)currentMode] 
        << " | Tile: (" << hoveredTileX << ", " << hoveredTileY << ")"
        << " | Value: " << currentTileValue
        << " | Zoom: " << zoomPercent << "%";
    
    if (currentMode == EditorMode::Erase) {
        oss << " | DELETE mode: Click ANY tile ? floor";
    } else if (currentMode == EditorMode::Wall) {
        oss << " | WALL mode: Click ANY tile ? wall (BUILD anywhere!)";
    } else {
        oss << " | PLACE mode: Click floor only | Use Delete (5) or Wall (1)";
    }
    
    statusText->SetText(oss.str().c_str());

    // Highlight selected button
    wallButton->SetBackgroundColor(currentMode == EditorMode::Wall ? 
        Engine::Color(0.7f, 0.7f, 0.7f, 1.0f) : Engine::Color(0.5f, 0.5f, 0.5f, 1.0f));
    doorButton->SetBackgroundColor(currentMode == EditorMode::Door ? 
        Engine::Color(0.4f, 0.7f, 1.0f, 1.0f) : Engine::Color(0.2f, 0.5f, 0.8f, 1.0f));
    terminalButton->SetBackgroundColor(currentMode == EditorMode::Terminal ? 
        Engine::Color(0.4f, 0.7f, 1.0f, 1.0f) : Engine::Color(0.2f, 0.5f, 0.8f, 1.0f));
    exitButton->SetBackgroundColor(currentMode == EditorMode::Exit ? 
        Engine::Color(0.4f, 0.7f, 1.0f, 1.0f) : Engine::Color(0.2f, 0.5f, 0.8f, 1.0f));
    eraseButton->SetBackgroundColor(currentMode == EditorMode::Erase ? 
        Engine::Color(1.0f, 0.5f, 0.5f, 1.0f) : Engine::Color(0.8f, 0.3f, 0.3f, 1.0f));
}

void LevelEditor::ToggleEditor() {
    isActive = !isActive;
    
    if (toolPanel) toolPanel->visible = isActive;
    if (mapViewPanel) mapViewPanel->visible = isActive;
    if (statusText) statusText->visible = isActive;
    if (helpText) helpText->visible = isActive;

    if (isActive) {
        if (Input::IsCursorLocked()) {
            Input::UnregisterCursorLock("FPSController");
        }
        Application::RegisterPause("LevelEditor");
        
        // Reset view to show full map
        viewOffsetX = 0.0f;
        viewOffsetY = 0.0f;
        viewZoom = 1.0f;
        
        std::cout << "[LevelEditor] Opened - Map size: " << mapWidth << "x" << mapHeight << "\n";
        std::cout << "[LevelEditor] Changes apply to TileMap in real-time!\n";
        std::cout << "[LevelEditor] Close editor (F1) to test changes in-game\n";
    } else {
        Input::RegisterCursorLock("FPSController");
        Application::UnregisterPause("LevelEditor");
        std::cout << "[LevelEditor] Closed - changes are active! Test them now.\n";
        std::cout << "[LevelEditor] Press S to save to map.txt for persistence\n";
    }
}

void LevelEditor::Render() {
    if (!isActive || !tileMap || !tileMesh) return;
    
    RenderMap();
}

void LevelEditor::RenderMap() {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    
    int screenWidth = AEGfxGetWindowWidth();
    int screenHeight = AEGfxGetWindowHeight();
    
    float scaledTileSize = tileSize * viewZoom;
    float mapOriginX = -mapWidth * scaledTileSize / 2.0f + viewOffsetX;
    float mapOriginY = -mapHeight * scaledTileSize / 2.0f + viewOffsetY;
    
    // Render each tile - standard orientation matching map.txt exactly
    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            int tile = (int)tileMap->GetTile(x, y);
            
            // Determine color (BRIGHTER COLORS)
            Engine::Color tileColor;
            if (tile == 0) {
                tileColor = Engine::Color(0.35f, 0.35f, 0.35f, 1.0f); // Floor - medium gray (brighter)
            } else if (tile == 1) {
                tileColor = Engine::Color(0.85f, 0.85f, 0.85f, 1.0f); // Wall - very light gray (brighter)
            } else if (tile == 10) {
                tileColor = Engine::Color(0.4f, 0.8f, 1.0f, 1.0f); // Door - bright blue
            } else if (tile == 11) {
                tileColor = Engine::Color(1.0f, 0.8f, 0.4f, 1.0f); // Terminal - bright orange
            } else if (tile == 12) {
                tileColor = Engine::Color(0.4f, 1.0f, 0.4f, 1.0f); // Exit - bright green
            } else {
                tileColor = Engine::Color(1.0f, 0.4f, 1.0f, 1.0f); // Other - bright purple
            }
            
            // Highlight hovered tile (even brighter)
            if (x == hoveredTileX && y == hoveredTileY) {
                tileColor.r = std::min(tileColor.r + 0.2f, 1.0f);
                tileColor.g = std::min(tileColor.g + 0.2f, 1.0f);
                tileColor.b = std::min(tileColor.b + 0.2f, 1.0f);
            }
            
            AEGfxSetColorToMultiply(tileColor.r, tileColor.g, tileColor.b, tileColor.a);
            AEGfxSetColorToAdd(0, 0, 0, 0);
            
            // Position - standard X/Y mapping (map.txt layout)
            // X increases left-to-right (columns 0-127)
            // Y increases top-to-bottom (rows 0-42)
            float posX = mapOriginX + x * scaledTileSize;
            float posY = mapOriginY + y * scaledTileSize;
            
            AEMtx33 transform;
            AEMtx33 scale, trans;
            AEMtx33Scale(&scale, scaledTileSize, scaledTileSize);
            AEMtx33Trans(&trans, posX, posY);
            AEMtx33Concat(&transform, &trans, &scale);
            
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(tileMesh, AE_GFX_MDM_TRIANGLES);
        }
    }
    
    // Draw grid lines
    RenderMapGrid();
}

void LevelEditor::RenderMapGrid() {
    // Grid rendering disabled for performance with large map (128x43)
    // Can be enabled for smaller maps if needed
}

void LevelEditor::SaveMapTxt(const std::string& filename) {
    if (!tileMap) {
        std::cout << "[LevelEditor] No tilemap to save!\n";
        return;
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cout << "[LevelEditor] Failed to save map to " << filename << "\n";
        return;
    }

    // Write the map row by row
    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            int tile = (int)tileMap->GetTile(x, y);
            char c = '0';
            
            if (tile == 0) c = '0';       // Floor
            else if (tile == 1) c = '1';  // Wall
            else if (tile == 10) c = 'D'; // Door
            else if (tile == 11) c = 'T'; // Terminal
            else if (tile == 12) c = 'E'; // Exit
            else if (tile == 13) c = 'I'; // Interactable
            else if (tile == 14) c = 'C'; // Control Room
            else c = '0'; // Fallback
            
            file << c;
        }
        file << '\n';
    }

    file.close();
    std::cout << "\n=== MAP SAVED ===";
    std::cout << "\n[LevelEditor] Successfully saved to " << filename;
    std::cout << "\n[LevelEditor] File updated: Assets/map.txt";
    std::cout << "\n[LevelEditor] RESTART the game to load these changes permanently!";
    std::cout << "\n[LevelEditor] Current session changes are live, but won't persist without restart.";
    std::cout << "\n=================\n";
}

void LevelEditor::SpawnDoorAt(int x, int y) {
    if (!scene || !tileMap) {
        std::cout << "[LevelEditor] ERROR: No scene/tilemap available for spawning!\n";
        return;
    }
    
    Vector2 pos(static_cast<f32>(x) + 0.5f, static_cast<f32>(y) + 0.5f);
    
    std::cout << "[LevelEditor] SPAWNING door at (" << pos.x << ", " << pos.y << ") in real-time!\n";
    
    // Determine door facing based on surrounding walls
    bool wallNorth = (y + 1 < mapHeight) && tileMap->IsWall(x, y + 1);
    bool wallSouth = (y - 1 >= 0) && tileMap->IsWall(x, y - 1);
    
    DoorFacing facing;
    if (wallNorth || wallSouth) {
        facing = DoorFacing::EastWest;
    } else {
        facing = DoorFacing::NorthSouth;
    }
    
    // Create door GameObject
    std::string doorName = "Door_Live_" + std::to_string(x) + "_" + std::to_string(y);
    auto doorObj = scene->CreateGameObject(doorName);
    doorObj->transform->position = Vector3(pos.x, pos.y, 0.5f);
    
    // Add BoxCollider2D FIRST
    auto collider = doorObj->AddComponent<BoxCollider2D>();
    collider->minHeight = 0.0f;
    collider->maxHeight = 2.0f;
    collider->isTrigger = false;
    
    // Set collider size based on facing (before Door component needs it)
    if (facing == DoorFacing::NorthSouth) {
        collider->size = Vector2(1.0f, 0.4f);
    } else {
        collider->size = Vector2(0.4f, 1.0f);
    }
    
    // Add Door component (will cache the collider in Awake)
    auto door = doorObj->AddComponent<Door>();
    door->tileX = x;
    door->tileY = y;
    door->doorType = 10;
    door->facing = facing;
    door->collider = collider;  // Explicitly set collider reference
    door->autoCloseDelay = 3.0f;  // Auto-close after 3 seconds
    
    // Add DoorRenderer
    auto doorRenderer = doorObj->AddComponent<DoorRenderer>();
    doorRenderer->fov = camera ? camera->fov : 75.0f;
    
    // Try to get wall sprite from TileMapRenderer
    if (auto worldObj = scene->Find("World")) {
        if (auto tmRenderer = worldObj->GetComponent<TileMapRenderer>()) {
            doorRenderer->sprite = tmRenderer->GetWallSprite(1);
        }
    }
    
    // Add Interactable
    auto interactable = doorObj->AddComponent<Interactable>();
    interactable->actionText = "Open";
    interactable->maxDistance = 2.5f;
    door->SetupInteractable(interactable, "Open", "Close");
    
    std::cout << "[LevelEditor] Door spawned! It will auto-close 3 seconds after opening.\n";
}

void LevelEditor::SpawnTerminalAt(int x, int y) {
    if (!scene) {
        std::cout << "[LevelEditor] ERROR: No scene available for spawning!\n";
        return;
    }
    
    // Get zone ID for this position
    int zoneId = 1; // Default fallback
    // TODO: Get actual zone from FPSScene if needed
    
    Vector2 pos(static_cast<f32>(x) + 0.5f, static_cast<f32>(y) + 0.5f);
    
    std::cout << "[LevelEditor] SPAWNING terminal at (" << pos.x << ", " << pos.y << ") in real-time!\n";
    
    // Create terminal GameObject
    auto terminalObj = scene->CreateGameObject("CCTVTerminal_Live_" + std::to_string(x) + "_" + std::to_string(y));
    terminalObj->transform->position = Vector3(pos.x, pos.y, 0.5f);
    terminalObj->transform->localScale = Vector2(0.5f, 0.5f);
    
    // Add sprite renderer
    auto sr = terminalObj->AddComponent<SpriteRenderer>();
    auto terminalSprite = AssetManager::GetSprite("Assets/powerup3.png");
    sr->SetSprite(terminalSprite);
    sr->billboard = true;
    sr->sortingOrder = 250;
    sr->highlightColor = Engine::Color(0.3f, 0.3f, 0.0f, 0.0f); // Yellow glow
    
    // Add CCTVTerminal component ONLY (no Door component)
    // CCTVTerminal.Awake() will automatically create Interactable component for player interaction
    auto terminal = terminalObj->AddComponent<CCTVTerminal>();
    terminal->controlledZone = zoneId;
    // Note: playerHUD, allGuards, controlledCameras are set by FPSScene on full game load
    // This spawns a basic interactive terminal for immediate testing
    
    std::cout << "[LevelEditor] Terminal spawned with Interactable (NO Door component)!\n";
    std::cout << "[LevelEditor] Close editor (F1) to interact. Save (S) + Restart for full functionality.\n";
}

} // namespace Game
