#include "LevelEditor.hpp"
/*!
 * \file LevelEditor.cpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief Implementation of the top-down level editor used in-game
 */
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
#include <unordered_map>
#include "Engine/Core/ConfigManager.hpp"

#undef min
#undef max

namespace
{
    /*! \struct LevelEditorConfig
     *  \brief JSON-backed configuration for the level editor UI and limits
     */
    struct LevelEditorConfig : public Engine::IConfig
    {
        float zoomInFactor = 1.2f;
        float zoomOutFactor = 0.8f;
        float zoomMin = 0.25f;
        float zoomMax = 4.0f;
        std::string defaultSavePath = "Assets/map.txt";

        /*! \brief Load configuration values from JSON. \param j [in] JSON node */
        void FromJson(const Json::Value& j) override
        {
            zoomInFactor = j.get("zoomInFactor", zoomInFactor).asFloat();
            zoomOutFactor = j.get("zoomOutFactor", zoomOutFactor).asFloat();
            zoomMin = j.get("zoomMin", zoomMin).asFloat();
            zoomMax = j.get("zoomMax", zoomMax).asFloat();
            defaultSavePath = j.get("defaultSavePath", defaultSavePath).asString();
        }
    };

    LevelEditorConfig g_levelEditorConfig;
    bool g_levelEditorConfigLoaded = false;

    /*! \brief Access singleton LevelEditorConfig, loads on first call. \return Reference to config */
    const LevelEditorConfig& GetLevelEditorConfig()
    {
        if (!g_levelEditorConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/level_editor.json", g_levelEditorConfig);
            if (g_levelEditorConfig.zoomMin < 0.01f) g_levelEditorConfig.zoomMin = 0.01f;
            if (g_levelEditorConfig.zoomMax < g_levelEditorConfig.zoomMin) g_levelEditorConfig.zoomMax = g_levelEditorConfig.zoomMin;
            if (g_levelEditorConfig.zoomInFactor <= 0.0f) g_levelEditorConfig.zoomInFactor = 1.2f;
            if (g_levelEditorConfig.zoomOutFactor <= 0.0f) g_levelEditorConfig.zoomOutFactor = 0.8f;
            g_levelEditorConfigLoaded = true;
        }
        return g_levelEditorConfig;
    }
}

namespace Game {

/*!
 * \brief Construct a LevelEditor instance.
 */
LevelEditor::LevelEditor() {}

/*!
 * \brief Destructor; free created mesh resources.
 */
LevelEditor::~LevelEditor() {
    if (tileMesh) {
        AEGfxMeshFree(tileMesh);
    }
    if (editorCanvas && UI::UIManager::GetInstance()) {
        // Cleanup handled by UIManager
    }
}

/*!
 * \brief Initialize the editor for a scene and tilemap.
 * \param scn [in] Scene pointer
 * \param map [in] TileMap pointer
 * \param cam [in] Camera pointer
 * \param playerTf [in] Player transform pointer
 */
/*!
 * \brief [AUTO] Initialize
 * \param scn [in]
 * \param map [in]
 * \param cam [in]
 * \param playerTf [in]
 */
/*!
 * \brief [AUTO] Initialize
 * \param scn [in]
 * \param map [in]
 * \param cam [in]
 * \param playerTf [in]
 */
/*!
 * \brief [AUTO] Initialize
 * \param scn [in]
 * \param map [in]
 * \param cam [in]
 * \param playerTf [in]
 */
/*!
 * \brief [AUTO] Initialize
 * \param scn [in]
 * \param map [in]
 * \param cam [in]
 * \param playerTf [in]
 */
/*!
 * \brief [AUTO] Initialize
 * \param scn [in]
 * \param map [in]
 * \param cam [in]
 * \param playerTf [in]
 */
void LevelEditor::Initialize(Scene* scn, TileMap* map, Camera* cam, Transform* playerTf) {
    // Re-initialize safety for scene reloads (FPS can be loaded multiple times)
    if (isActive) {
        Input::UnregisterCursorUnlock("LevelEditor");
        Application::UnregisterPause("LevelEditor");
        isActive = false;
    }

    if (tileMesh) {
        AEGfxMeshFree(tileMesh);
        tileMesh = nullptr;
    }

    if (editorCanvas && UI::UIManager::GetInstance()) {
        UI::UIManager::GetInstance()->DestroyCanvas(editorCanvas);
        editorCanvas = nullptr;
        toolPanel = nullptr;
        mapViewPanel = nullptr;
        statusText = nullptr;
        helpText = nullptr;
        wallButton = nullptr;
        doorButton = nullptr;
        terminalButton = nullptr;
        exitButton = nullptr;
        controlRoomButton = nullptr;
        greenCardButton = nullptr;
        purpleCardButton = nullptr;
        redCardButton = nullptr;
        yellowCardButton = nullptr;
        eraseButton = nullptr;
        saveButton = nullptr;
        closeButton = nullptr;
    }

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

/*!
 * \brief Create UI elements for the editor (buttons, panels, help text).
 */
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
    terminalButton->SetSizeDelta({100.0f, 0.0f});
    terminalButton->SetText("Term (3)");
    terminalButton->SetBackgroundColor(Engine::Color(1.0f, 0.8f, 0.2f, 1.0f));
    terminalButton->onClick = [this]() { SetMode(EditorMode::Terminal); };

    exitButton = buttonLayout->CreateChild<UI::Button>();
    exitButton->SetSizeDelta({80.0f, 0.0f});
    exitButton->SetText("Exit (4)");
    exitButton->SetBackgroundColor(Engine::Color(0.2f, 0.8f, 0.2f, 1.0f));
    exitButton->onClick = [this]() { SetMode(EditorMode::Exit); };

    controlRoomButton = buttonLayout->CreateChild<UI::Button>();
    controlRoomButton->SetSizeDelta({80.0f, 0.0f});
    controlRoomButton->SetText("Ctrl (5)");
    controlRoomButton->SetBackgroundColor(Engine::Color(0.8f, 0.2f, 0.8f, 1.0f));
    controlRoomButton->onClick = [this]() { SetMode(EditorMode::ControlRoom); };

    greenCardButton = buttonLayout->CreateChild<UI::Button>();
    greenCardButton->SetSizeDelta({80.0f, 0.0f});
    greenCardButton->SetText("Grn (6)");
    greenCardButton->SetBackgroundColor(Engine::Color(0.2f, 0.8f, 0.2f, 1.0f));
    greenCardButton->onClick = [this]() { SetMode(EditorMode::GreenCard); };

    purpleCardButton = buttonLayout->CreateChild<UI::Button>();
    purpleCardButton->SetSizeDelta({80.0f, 0.0f});
    purpleCardButton->SetText("Prp (7)");
    purpleCardButton->SetBackgroundColor(Engine::Color(0.8f, 0.2f, 0.8f, 1.0f));
    purpleCardButton->onClick = [this]() { SetMode(EditorMode::PurpleCard); };

    redCardButton = buttonLayout->CreateChild<UI::Button>();
    redCardButton->SetSizeDelta({80.0f, 0.0f});
    redCardButton->SetText("Red (8)");
    redCardButton->SetBackgroundColor(Engine::Color(0.8f, 0.2f, 0.2f, 1.0f));
    redCardButton->onClick = [this]() { SetMode(EditorMode::RedCard); };

    yellowCardButton = buttonLayout->CreateChild<UI::Button>();
    yellowCardButton->SetSizeDelta({80.0f, 0.0f});
    yellowCardButton->SetText("Ylw (9)");
    yellowCardButton->SetBackgroundColor(Engine::Color(0.8f, 0.8f, 0.2f, 1.0f));
    yellowCardButton->onClick = [this]() { SetMode(EditorMode::YellowCard); };

    eraseButton = buttonLayout->CreateChild<UI::Button>();
    eraseButton->SetSizeDelta({80.0f, 0.0f});
    eraseButton->SetText("Del (0)");
    eraseButton->SetBackgroundColor(Engine::Color(0.8f, 0.3f, 0.3f, 1.0f));
    eraseButton->onClick = [this]() { SetMode(EditorMode::Erase); };

    // Save button
    saveButton = buttonLayout->CreateChild<UI::Button>();
    saveButton->SetSizeDelta({120.0f, 0.0f});
    saveButton->SetText("Save (S)");
    saveButton->SetBackgroundColor(Engine::Color(0.3f, 0.7f, 0.3f, 1.0f));
    saveButton->onClick = [this]() { SaveMapTxt(GetLevelEditorConfig().defaultSavePath); };

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
        "1-9,0: Select mode\n"
        "1: Wall 2: Door 3: Term\n"
        "4: Exit 5: Ctrl Room\n"
        "6: Grn 7: Prp 8: Red 9: Ylw\n"
        "0: DELETE any tile\n"
        "K: Control Room (quick!)\n"
        "S: Save map.txt\n"
        "F1: Close editor"
    );
    helpText->SetFontSize(1.0f);
    helpText->SetColor(Engine::Color(1.0f, 1.0f, 0.0f, 1.0f));
    helpText->SetAnchor(UI::TextAnchor::UpperRight);
    helpText->visible = false;
}

/*!
 * \brief Per-frame update for the editor when active.
 */
void LevelEditor::Update() {
    if (!isActive) return;
    
    HandleInput();
    UpdateUI();
}

/*!
 * \brief Handle keyboard and mouse input for editor controls.
 */
void LevelEditor::HandleInput() {
// Mode selection
if (Input::GetKeyDown(KeyCode::Alpha1)) SetMode(EditorMode::Wall);
if (Input::GetKeyDown(KeyCode::Alpha2)) SetMode(EditorMode::Door);
if (Input::GetKeyDown(KeyCode::Alpha3)) SetMode(EditorMode::Terminal);
if (Input::GetKeyDown(KeyCode::Alpha4)) SetMode(EditorMode::Exit);
if (Input::GetKeyDown(KeyCode::Alpha5)) SetMode(EditorMode::ControlRoom);
if (Input::GetKeyDown(KeyCode::Alpha6)) SetMode(EditorMode::GreenCard);
if (Input::GetKeyDown(KeyCode::Alpha7)) SetMode(EditorMode::PurpleCard);
if (Input::GetKeyDown(KeyCode::Alpha8)) SetMode(EditorMode::RedCard);
if (Input::GetKeyDown(KeyCode::Alpha9)) SetMode(EditorMode::YellowCard);
if (Input::GetKeyDown(KeyCode::Alpha0)) SetMode(EditorMode::Erase);

// Alternative: Press K for Control Room Door (easier to remember!)
if (Input::GetKeyDown(KeyCode::K)) SetMode(EditorMode::ControlRoom);

    // Save
    if (Input::GetKeyDown(KeyCode::S)) {
        SaveMapTxt(GetLevelEditorConfig().defaultSavePath);
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
        const auto& cfg = GetLevelEditorConfig();
        viewZoom = std::min(viewZoom * cfg.zoomInFactor, cfg.zoomMax);
    }
    if (Input::GetKeyDown(KeyCode::Minus)) {
        const auto& cfg = GetLevelEditorConfig();
        viewZoom = std::max(viewZoom * cfg.zoomOutFactor, cfg.zoomMin);
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

/*!
 * \brief Apply the current editor action to the hovered tile when clicked.
 */
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
        // Remove any special markers stored at this location (exit/terminal/control room/cards)
        if (tileMap) {
            tileMap->RemoveExitAt(hoveredTileX, hoveredTileY);
        }
        return;
    }
    
    // Wall mode - convert ANY tile to wall (can overwrite anything)
    if (currentMode == EditorMode::Wall) {
        tileMap->SetTile(hoveredTileX, hoveredTileY, Engine::TileType::Wall);
        std::cout << "[LevelEditor] Built wall at (" << hoveredTileX << ", " << hoveredTileY 
                  << ") (LIVE UPDATE)\n";
        // Placing a wall should remove any exit marker present here
        if (tileMap) {
            tileMap->RemoveExitAt(hoveredTileX, hoveredTileY);
        }
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
            SpawnDoorAt(hoveredTileX, hoveredTileY);
            break;
        case EditorMode::Terminal:
            newTile = static_cast<Engine::TileType>(11); // Terminal marker
            std::cout << "[LevelEditor] Placed terminal at (" << hoveredTileX << ", " << hoveredTileY << ") (SPAWNING NOW)\n";
            SpawnTerminalAt(hoveredTileX, hoveredTileY);
            break;
        case EditorMode::Exit:
            newTile = static_cast<Engine::TileType>(12); // Exit marker
            std::cout << "[LevelEditor] Placed exit at (" << hoveredTileX << ", " << hoveredTileY << ") (LIVE UPDATE)\n";
            if (tileMap) tileMap->AddExitAt(hoveredTileX, hoveredTileY);
            break;
        case EditorMode::ControlRoom:
            newTile = static_cast<Engine::TileType>(20); // Control room door marker (NEW 'K' marker)
            std::cout << "[LevelEditor] Placed control room door at (" << hoveredTileX << ", " << hoveredTileY << ") (RESTART REQUIRED)\n";
            break;
        case EditorMode::GreenCard:
            newTile = static_cast<Engine::TileType>(15); // Green card marker (F)
            std::cout << "[LevelEditor] Placed GREEN access card at (" << hoveredTileX << ", " << hoveredTileY << ") (RESTART REQUIRED)\n";
            break;
        case EditorMode::PurpleCard:
            newTile = static_cast<Engine::TileType>(16); // Purple card marker (G)
            std::cout << "[LevelEditor] Placed PURPLE access card at (" << hoveredTileX << ", " << hoveredTileY << ") (RESTART REQUIRED)\n";
            break;
        case EditorMode::RedCard:
            newTile = static_cast<Engine::TileType>(17); // Red card marker (H)
            std::cout << "[LevelEditor] Placed RED access card at (" << hoveredTileX << ", " << hoveredTileY << ") (RESTART REQUIRED)\n";
            break;
        case EditorMode::YellowCard:
            newTile = static_cast<Engine::TileType>(18); // Yellow card marker (J)
            std::cout << "[LevelEditor] Placed YELLOW access card at (" << hoveredTileX << ", " << hoveredTileY << ") (RESTART REQUIRED)\n";
            break;
        case EditorMode::Wall:
        case EditorMode::Erase:
            // Already handled above
            break;
    }
    
    tileMap->SetTile(hoveredTileX, hoveredTileY, newTile);

    // If we've just set the tile to empty (e.g. via Erase) remove any exit stored here
    if (newTile == Engine::TileType::Empty) {
        if (tileMap) tileMap->RemoveExitAt(hoveredTileX, hoveredTileY);
    }
    
    // Note: Changes are applied to TileMap immediately
    // Renderer and collider read from TileMap each frame automatically
    // Close editor (F1) to see changes in-game
}

/*!
 * \brief Convert screen coordinates to map tile-space coordinates.
 * \param screenX [in] Screen X in pixels
 * \param screenY [in] Screen Y in pixels
 * \return Map coordinates as a Vector2
 */
/*!
 * \brief [AUTO] ScreenToMapCoords
 * \param screenX [in]
 * \param screenY [in]
 * \return 
 */
/*!
 * \brief [AUTO] ScreenToMapCoords
 * \param screenX [in]
 * \param screenY [in]
 * \return 
 */
/*!
 * \brief [AUTO] ScreenToMapCoords
 * \param screenX [in]
 * \param screenY [in]
 * \return 
 */
/*!
 * \brief [AUTO] ScreenToMapCoords
 * \param screenX [in]
 * \param screenY [in]
 * \return 
 */
/*!
 * \brief [AUTO] ScreenToMapCoords
 * \param screenX [in]
 * \param screenY [in]
 * \return 
 */
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

/*!
 * \brief Test whether a tile is floor (editable placement target).
 * \param x [in] Tile X
 * \param y [in] Tile Y
 * \return true when tile is floor and within bounds
 */
/*!
 * \brief [AUTO] IsFloorTile
 * \param x [in]
 * \param y [in]
 * \return 
 */
/*!
 * \brief [AUTO] IsFloorTile
 * \param x [in]
 * \param y [in]
 * \return 
 */
/*!
 * \brief [AUTO] IsFloorTile
 * \param x [in]
 * \param y [in]
 * \return 
 */
/*!
 * \brief [AUTO] IsFloorTile
 * \param x [in]
 * \param y [in]
 * \return 
 */
/*!
 * \brief [AUTO] IsFloorTile
 * \param x [in]
 * \param y [in]
 * \return 
 */
bool LevelEditor::IsFloorTile(int x, int y) {
    if (!tileMap) return false;
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) return false;
    return (int)tileMap->GetTile(x, y) == 0;
}

/*!
 * \brief Set the editor placement mode.
 * \param mode [in] Desired EditorMode
 */
/*!
 * \brief [AUTO] SetMode
 * \param mode [in]
 */
/*!
 * \brief [AUTO] SetMode
 * \param mode [in]
 */
/*!
 * \brief [AUTO] SetMode
 * \param mode [in]
 */
/*!
 * \brief [AUTO] SetMode
 * \param mode [in]
 */
/*!
 * \brief [AUTO] SetMode
 * \param mode [in]
 */
void LevelEditor::SetMode(EditorMode mode) {
    currentMode = mode;
    const char* modeNames[] = {"Wall", "Door", "Terminal", "Exit", "ControlRoom", "GreenCard", "PurpleCard", "RedCard", "YellowCard", "Delete"};
    std::cout << "[LevelEditor] Mode: " << modeNames[(int)mode] << "\n";
}

/*!
 * \brief Refresh UI text and button highlights based on current state.
 */
void LevelEditor::UpdateUI() {
if (!statusText) return;

const char* modeNames[] = {"Wall", "Door", "Terminal", "Exit", "ControlRoom", "GreenCard", "PurpleCard", "RedCard", "YellowCard", "Delete"};
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
        Engine::Color(1.0f, 1.0f, 0.4f, 1.0f) : Engine::Color(1.0f, 0.8f, 0.2f, 1.0f));
    exitButton->SetBackgroundColor(currentMode == EditorMode::Exit ? 
        Engine::Color(0.4f, 1.0f, 0.4f, 1.0f) : Engine::Color(0.2f, 0.8f, 0.2f, 1.0f));
    controlRoomButton->SetBackgroundColor(currentMode == EditorMode::ControlRoom ? 
        Engine::Color(1.0f, 0.4f, 1.0f, 1.0f) : Engine::Color(0.8f, 0.2f, 0.8f, 1.0f));
    greenCardButton->SetBackgroundColor(currentMode == EditorMode::GreenCard ? 
        Engine::Color(0.4f, 1.0f, 0.4f, 1.0f) : Engine::Color(0.2f, 0.8f, 0.2f, 1.0f));
    purpleCardButton->SetBackgroundColor(currentMode == EditorMode::PurpleCard ? 
        Engine::Color(1.0f, 0.4f, 1.0f, 1.0f) : Engine::Color(0.8f, 0.2f, 0.8f, 1.0f));
    redCardButton->SetBackgroundColor(currentMode == EditorMode::RedCard ? 
        Engine::Color(1.0f, 0.4f, 0.4f, 1.0f) : Engine::Color(0.8f, 0.2f, 0.2f, 1.0f));
    yellowCardButton->SetBackgroundColor(currentMode == EditorMode::YellowCard ? 
        Engine::Color(1.0f, 1.0f, 0.4f, 1.0f) : Engine::Color(0.8f, 0.8f, 0.2f, 1.0f));
    eraseButton->SetBackgroundColor(currentMode == EditorMode::Erase ? 
        Engine::Color(1.0f, 0.5f, 0.5f, 1.0f) : Engine::Color(0.8f, 0.3f, 0.3f, 1.0f));
}

/*!
 * \brief [AUTO] ToggleEditor
 */
void LevelEditor::ToggleEditor() {
    isActive = !isActive;
    
    if (toolPanel) toolPanel->visible = isActive;
    if (mapViewPanel) mapViewPanel->visible = isActive;
    if (statusText) statusText->visible = isActive;
    if (helpText) helpText->visible = isActive;

    if (isActive) {
        Input::RegisterCursorUnlock("LevelEditor");
        Application::RegisterPause("LevelEditor");
        
        // Reset view to show full map
        viewOffsetX = 0.0f;
        viewOffsetY = 0.0f;
        viewZoom = 1.0f;
        
        std::cout << "[LevelEditor] Opened - Map size: " << mapWidth << "x" << mapHeight << "\n";
        std::cout << "[LevelEditor] Changes apply to TileMap in real-time!\n";
        std::cout << "[LevelEditor] Close editor (F1) to test changes in-game\n";
    } else {
        Input::UnregisterCursorUnlock("LevelEditor");
        Application::UnregisterPause("LevelEditor");
        std::cout << "[LevelEditor] Closed - changes are active! Test them now.\n";
        std::cout << "[LevelEditor] Press S to save to map.txt for persistence\n";
    }
}

/*!
 * \brief [AUTO] Render
 */
void LevelEditor::Render() {
    if (!isActive || !tileMap || !tileMesh) return;
    
    RenderMap();
}

/*!
 * \brief [AUTO] RenderMap
 */
void LevelEditor::RenderMap() {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    
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
                tileColor = Engine::Color(0.35f, 0.35f, 0.35f, 1.0f); // Floor - medium gray
            } else if (tile == 1) {
                tileColor = Engine::Color(0.85f, 0.85f, 0.85f, 1.0f); // Wall - very light gray
            } else if (tile == 10) {
                tileColor = Engine::Color(0.4f, 0.8f, 1.0f, 1.0f); // Door - bright blue
            } else if (tile == 11) {
                tileColor = Engine::Color(1.0f, 0.8f, 0.4f, 1.0f); // Terminal - bright orange
            } else if (tile == 12) {
                tileColor = Engine::Color(0.4f, 1.0f, 0.4f, 1.0f); // Exit - bright green
            } else if (tile == 13) {
                tileColor = Engine::Color(1.0f, 0.4f, 1.0f, 1.0f); // Control room door (old) - magenta
            } else if (tile == 20) {
                tileColor = Engine::Color(1.0f, 0.2f, 1.0f, 1.0f); // Control room door (NEW) - bright magenta
            } else if (tile == 15) {
                tileColor = Engine::Color(0.2f, 1.0f, 0.2f, 1.0f); // Green card - bright green
            } else if (tile == 16) {
                tileColor = Engine::Color(0.8f, 0.2f, 1.0f, 1.0f); // Purple card - bright purple
            } else if (tile == 17) {
                tileColor = Engine::Color(1.0f, 0.2f, 0.2f, 1.0f); // Red card - bright red
            } else if (tile == 18) {
                tileColor = Engine::Color(1.0f, 1.0f, 0.2f, 1.0f); // Yellow card - bright yellow
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

/*!
 * \brief [AUTO] RenderMapGrid
 */
void LevelEditor::RenderMapGrid() {
    // Grid rendering disabled for performance with large map (128x43)
    // Can be enabled for smaller maps if needed
}

/*!
 * \brief [AUTO] SaveMapTxt
 * \param filename [in]
 */
/*!
 * \brief [AUTO] SaveMapTxt
 * \param filename [in]
 */
/*!
 * \brief [AUTO] SaveMapTxt
 * \param filename [in]
 */
/*!
 * \brief [AUTO] SaveMapTxt
 * \param filename [in]
 */
/*!
 * \brief [AUTO] SaveMapTxt
 * \param filename [in]
 */
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

    // Build overlay map from stored marker positions.
    // LoadFromArray replaces special markers (doors, terminals, exits, cards, etc.)
    // with Empty in the live tiles array, so GetTile() returns 0 for those.
    // We must re-inject them from the stored position vectors before saving.
    std::unordered_map<int, char> overlay; // key = y*width+x, value = map char

    // Spawned doors (tile 10 ? 'D')
    for (auto* door : tileMap->GetDoors()) {
        if (!door) continue;
        int idx = door->tileY * mapWidth + door->tileX;
        overlay[idx] = 'D';
    }

    // Terminal positions (tile 11 ? 'T')
    // for (auto& pos : tileMap->GetTerminalPositions()) {
    //     int tx = (int)std::floor(pos.x);
    //     int ty = (int)std::floor(pos.y);
    //     if (tx >= 0 && tx < mapWidth && ty >= 0 && ty < mapHeight)
    //         overlay[ty * mapWidth + tx] = 'T';
    // }

    // Exit positions (tile 12 ? 'E')
    for (auto& pos : tileMap->GetExitPositions()) {
        int tx = (int)std::floor(pos.x);
        int ty = (int)std::floor(pos.y);
        if (tx >= 0 && tx < mapWidth && ty >= 0 && ty < mapHeight)
            overlay[ty * mapWidth + tx] = 'E';
    }

    // Control room door positions (tile 20 ? 'K')
    for (auto& pos : tileMap->GetControlRoomDoorPositions()) {
        int tx = (int)std::floor(pos.x);
        int ty = (int)std::floor(pos.y);
        if (tx >= 0 && tx < mapWidth && ty >= 0 && ty < mapHeight)
            overlay[ty * mapWidth + tx] = 'K';
    }

    // Access card positions (tiles 15-18 ? 'F','G','H','J')
    // const char cardChars[] = { 'F', 'G', 'H', 'J' };
    // for (auto& [pos, cardId] : tileMap->GetAccessCardPositions()) {
    //     int tx = (int)std::floor(pos.x);
    //     int ty = (int)std::floor(pos.y);
    //     if (tx >= 0 && tx < mapWidth && ty >= 0 && ty < mapHeight && cardId >= 0 && cardId <= 3)
    //         overlay[ty * mapWidth + tx] = cardChars[cardId];
    // }

    // Write the map row by row
    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            int idx = y * mapWidth + x;

            // Check overlay first (re-injected markers)
            auto it = overlay.find(idx);
            if (it != overlay.end()) {
                file << it->second;
                continue;
            }

            int tile = (int)tileMap->GetTile(x, y);
            char c = '0';
            
            if (tile == 0) c = '0';       // Floor
            else if (tile == 1) c = '1';  // Wall
            else if (tile == 10) c = 'D'; // Door
            else if (tile == 11) c = 'T'; // Terminal
            else if (tile == 12) c = 'E'; // Exit
            else if (tile == 13) c = 'C'; // Control Room Door (old)
            else if (tile == 20) c = 'K'; // Control Room Door (NEW!)
            else if (tile == 15) c = 'F'; // Green Access Card
            else if (tile == 16) c = 'G'; // Purple Access Card
            else if (tile == 17) c = 'H'; // Red Access Card
            else if (tile == 18) c = 'J'; // Yellow Access Card
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

/*!
 * \brief [AUTO] SpawnDoorAt
 * \param x [in]
 * \param y [in]
 */
/*!
 * \brief [AUTO] SpawnDoorAt
 * \param x [in]
 * \param y [in]
 */
/*!
 * \brief [AUTO] SpawnDoorAt
 * \param x [in]
 * \param y [in]
 */
/*!
 * \brief [AUTO] SpawnDoorAt
 * \param x [in]
 * \param y [in]
 */
/*!
 * \brief [AUTO] SpawnDoorAt
 * \param x [in]
 * \param y [in]
 */
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

/*! 
 * \brief Spawn a simple interactive door GameObject at tile coordinates (x,y).
 * \param x [in] Tile X coordinate
 * \param y [in] Tile Y coordinate
 */
/*!
 * \brief [AUTO] SpawnTerminalAt
 * \param x [in]
 * \param y [in]
 */
/*!
 * \brief [AUTO] SpawnTerminalAt
 * \param x [in]
 * \param y [in]
 */
/*!
 * \brief [AUTO] SpawnTerminalAt
 * \param x [in]
 * \param y [in]
 */
/*!
 * \brief [AUTO] SpawnTerminalAt
 * \param x [in]
 * \param y [in]
 */
/*!
 * \brief [AUTO] SpawnTerminalAt
 * \param x [in]
 * \param y [in]
 */
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
    auto terminalSprite = AssetManager::GetSprite("Assets/Sprites/powerup3.png");
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
