#include "MainMenuScene.hpp"

namespace Game {

MainMenuScene::MainMenuScene() : Scene("MainMenuScene") {}

void MainMenuScene::OnLoad() {
    // Build corridor map data:
    // Row 0: 111111... (wall - top)
    // Row 1: 000000... (walkable corridor - no end walls for infinite look)
    // Row 2: 111111... (wall - bottom)
    std::vector<int> corridorData(CORRIDOR_LENGTH * CORRIDOR_HEIGHT, 0);
    
    for (int x = 0; x < CORRIDOR_LENGTH; ++x) {
        // Top wall (row 0)
        corridorData[0 * CORRIDOR_LENGTH + x] = 1;
        // Bottom wall (row 2)
        corridorData[2 * CORRIDOR_LENGTH + x] = 1;
        // NO end walls - keep row 1 all zeros for infinite corridor appearance
    }
    
    // Create world with tilemap
    auto world = CreateGameObject("World");
    tileMap = world->AddComponent<TileMap>();
    tileMap->LoadFromArray(corridorData.data(), CORRIDOR_LENGTH, CORRIDOR_HEIGHT);
    
    // Set up renderer with same sprites as FPSScene
    auto renderer = world->AddComponent<TileMapRenderer>();
    renderer->SetCeilingSprite(AssetManager::GetSprite("Assets/Tilemap/modern_school_ceiling.png"));
    renderer->SetFloorSprite(AssetManager::GetSprite("Assets/Tilemap/modern_school_floor.png"));
    renderer->SetWallSprite(AssetManager::GetSprite("Assets/Tilemap/modern_school_wall.png"));
    
    // Create camera at center of corridor
    auto cameraGO = CreateGameObject("Camera");
    auto cam = cameraGO->AddComponent<Camera>();
    cameraTf = cameraGO->transform;
    
    // Set background to black (matches fog color, hides any gaps)
    cam->backgroundColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Enable fog for atmospheric effect
    cam->fogEnabled = true;
    cam->fogPrecision = 10000.f;
    cam->fogDistance = 0.5f;  // Fade to black over 8 units
    cam->fogMinVisibility = 0.f;  // Allow complete blackout at distance
	cam->fogMaxVisibility = 0.7f; // Max visibility close up
    
    // Start at center of corridor (away from edges to avoid wall rendering)
    cameraTf->position = Vector3(50.0f, 1.5f, 0.5f);
    cameraTf->rotation = 180.0f;  // Looking down the infinite corridor
    
    // Wrap boundaries: stay in center-to-end region (X=50 to X=90)
    // This keeps camera far from map edges where renderer shows walls
    wrapStart = 50.0f;  // Center of corridor
    wrapEnd = 90.0f;    // Before approaching the end (stays well within map)
}

void MainMenuScene::OnUpdate() {
    if (!cameraTf) return;
    
    // Move camera backward (in the +X direction, away from where camera is looking at 180°)
    cameraTf->position.x += moveSpeed * Time::unscaledDeltaTime;
    
    // Wrap around for infinite scroll (when reaching near end, jump back to center)
    if (cameraTf->position.x >= wrapEnd) {
        cameraTf->position.x = wrapStart + (cameraTf->position.x - wrapEnd);
    }
}

} // namespace Game
