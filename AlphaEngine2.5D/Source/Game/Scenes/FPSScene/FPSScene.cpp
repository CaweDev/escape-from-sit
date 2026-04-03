#define FPS_SCENE_CPP_COMPILATION_UNIT
#include "Game/Scenes/FPSScene/FPSScene.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <fstream>
#include <json/json.h>

/*!
 * \file FPSScene.cpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief Implementation of the FPSScene including map loading and game logic
 */

namespace
{
    /*!
     * \struct FPSSceneConfig
     * \brief JSON-backed configuration for gameplay tuning and spawn rules
     */
    struct FPSSceneConfig : public Engine::IConfig
    {
        float winRadius = 0.6f;
        float banditInterval = 2.0f;
        float wHeat = 0.55f;
        float wCCTV = 0.45f;
        float heatRef = 50.0f;
        float cctvRef = 6.0f;
        float heatAddPerSec = 1.0f;
        float heatDecayPerSec = 0.985f;
        float cctvSpikeCooldown = 1.0f;
        float cctvSpikeAmount = 1.0f;
        float fullEscalationThreshold = 30.0f;


     
        float npcNoticeRadius = 6.0f;
        float guardShakeDistance = 6.0f;
        float guardShakeDuration = 0.1f;
        float guardShakeIntensityScale = 0.08f;
        float mlSummaryInterval = 5.0f;

        float tileMapColliderMinHeight = 0.0f;
        float tileMapColliderMaxHeight = 3.0f;

        float debugCirclePi = 3.14159265f;
        float distanceSentinel = 999999.0f;
        float escalationStage2Ratio = 0.33f;
        float escalationStage3Ratio = 0.66f;

        Engine::Color wallColorNS = Engine::Color(0.8f, 0.2f, 0.2f, 1.0f);
        Engine::Color wallColorEW = Engine::Color(0.6f, 0.15f, 0.15f, 1.0f);
        Engine::Color ceilingColor = Engine::Color(0.1f, 0.1f, 0.2f, 1.0f);
        Engine::Color floorColor = Engine::Color(0.3f, 0.25f, 0.2f, 1.0f);

        /*!
         * \brief Parse a color from JSON with a fallback
         * \param j [in] JSON root
         * \param key [in] Key name for color object
         * \param fallback [in] Fallback color
         * \return Parsed color or fallback
         */
        static Engine::Color ParseColor(const Json::Value &j, const char *key, const Engine::Color &fallback)
        {
            if (!j.isMember(key) || !j[key].isObject())
                return fallback;
            const auto &c = j[key];
            return Engine::Color(
                c.get("r", fallback.r).asFloat(),
                c.get("g", fallback.g).asFloat(),
                c.get("b", fallback.b).asFloat(),
                c.get("a", fallback.a).asFloat());
        }

        /*!
         * \brief Populate config from JSON node
         * \param j [in] JSON node
         */
        void FromJson(const Json::Value &j) override
        {
            winRadius = j.get("winRadius", winRadius).asFloat();
            banditInterval = j.get("banditInterval", banditInterval).asFloat();
            wHeat = j.get("wHeat", wHeat).asFloat();
            wCCTV = j.get("wCCTV", wCCTV).asFloat();
            heatRef = j.get("heatRef", heatRef).asFloat();
            cctvRef = j.get("cctvRef", cctvRef).asFloat();
            heatAddPerSec = j.get("heatAddPerSec", heatAddPerSec).asFloat();
            heatDecayPerSec = j.get("heatDecayPerSec", heatDecayPerSec).asFloat();
            cctvSpikeCooldown = j.get("cctvSpikeCooldown", cctvSpikeCooldown).asFloat();
            cctvSpikeAmount = j.get("cctvSpikeAmount", cctvSpikeAmount).asFloat();
            fullEscalationThreshold = j.get("fullEscalationThreshold", fullEscalationThreshold).asFloat();


            npcNoticeRadius = j.get("npcNoticeRadius", npcNoticeRadius).asFloat();
            guardShakeDistance = j.get("guardShakeDistance", guardShakeDistance).asFloat();
            guardShakeDuration = j.get("guardShakeDuration", guardShakeDuration).asFloat();
            guardShakeIntensityScale = j.get("guardShakeIntensityScale", guardShakeIntensityScale).asFloat();
            mlSummaryInterval = j.get("mlSummaryInterval", mlSummaryInterval).asFloat();

            tileMapColliderMinHeight = j.get("tileMapColliderMinHeight", tileMapColliderMinHeight).asFloat();
            tileMapColliderMaxHeight = j.get("tileMapColliderMaxHeight", tileMapColliderMaxHeight).asFloat();
            debugCirclePi = j.get("debugCirclePi", debugCirclePi).asFloat();
            distanceSentinel = j.get("distanceSentinel", distanceSentinel).asFloat();
            escalationStage2Ratio = j.get("escalationStage2Ratio", escalationStage2Ratio).asFloat();
            escalationStage3Ratio = j.get("escalationStage3Ratio", escalationStage3Ratio).asFloat();

            wallColorNS = ParseColor(j, "wallColorNS", wallColorNS);
            wallColorEW = ParseColor(j, "wallColorEW", wallColorEW);
            ceilingColor = ParseColor(j, "ceilingColor", ceilingColor);
            floorColor = ParseColor(j, "floorColor", floorColor);
        }
    };

    FPSSceneConfig g_fpsSceneConfig;
    bool g_fpsSceneConfigLoaded = false;

    /*!
     * \brief Get singleton FPSSceneConfig, reading JSON on first call
     * \return Reference to FPSSceneConfig
     */
    const FPSSceneConfig &GetFPSSceneConfig()
    {
        if (!g_fpsSceneConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/fps_scene.json", g_fpsSceneConfig);
            g_fpsSceneConfigLoaded = true;
        }
        return g_fpsSceneConfig;
    }
}

namespace Game
{

    /*! \brief Load and initialize FPS scene: map, actors, HUD and systems. */
    inline void FPSScene::OnLoad()
    {
        AEEditor::ObjectManager::LoadSceneFromJson("Assets/Config/Scenes/EditorScene.json");

        const auto &cfg = GetFPSSceneConfig();

        winRadius = cfg.winRadius;
        banditInterval = cfg.banditInterval;
        wHeat = cfg.wHeat;
        wCCTV = cfg.wCCTV;
        heatRef = cfg.heatRef;
        cctvRef = cfg.cctvRef;
        heatAddPerSec = cfg.heatAddPerSec;
        heatDecayPerSec = cfg.heatDecayPerSec;
        cctvSpikeCooldown = cfg.cctvSpikeCooldown;
        cctvSpikeAmount = cfg.cctvSpikeAmount;
        fullEscalationThreshold = cfg.fullEscalationThreshold;

        gameEnded = false;
        printedWin = false;
        SoundManager::PlayBGM("fps_gameplay", true);
        // ===== load map.txt =====
        std::vector<std::string> mapRows;
        {
            std::ifstream file("Assets/tmpMap.txt");
            if (!file.is_open())
            {
                file.clear();
                file.open("Assets/map.txt");
            }

            if (!file.is_open())
                return;

            std::string line;
            while (std::getline(file, line))
            {
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                if (!line.empty())
                    mapRows.push_back(line);
            }
            file.close();
            if (mapRows.empty())
                return;
        }

        const int loadedMapHeight = (int)mapRows.size();
        const int loadedMapWidth = (int)mapRows[0].size();

        // Store for later use
        this->mapWidth = loadedMapWidth;
        this->mapHeight = loadedMapHeight;

        for (int y = 0; y < loadedMapHeight; ++y)
        {
            if ((int)mapRows[y].size() != loadedMapWidth)
                return;
        }

        // ===== zones (quadrants) =====
        // IMPORTANT: Minimap Y-axis is INVERTED from game coordinates!
        // Game coords: Y=0 is TOP, Y=max is BOTTOM
        // Minimap:     Y=0 is BOTTOM, Y=max is TOP
        // Zone layout (matching minimap colors when Y is INVERTED):
        //   Minimap Top-Left (Green):    Zone B (Game coords: Y=21-42, bottom-left)
        //   Minimap Top-Right (Magenta): Zone C (Game coords: Y=21-42, bottom-right)
        //   Minimap Bot-Left (Blue):     Zone A (Game coords: Y=0-20, top-left)
        //   Minimap Bot-Right (Yellow):  Zone D (Game coords: Y=0-20, top-right)
        zones.clear();
        int midX = loadedMapWidth / 2;
        int midY = loadedMapHeight / 2;

        zones.push_back({1, 0, midX - 1, 0, midY - 1});                            // A: Blue (Game top-left, Minimap bottom-left)
        zones.push_back({2, 0, midX - 1, midY, loadedMapHeight - 1});              // B: Green (Game bottom-left, Minimap top-left)
        zones.push_back({3, midX, loadedMapWidth - 1, midY, loadedMapHeight - 1}); // C: Magenta (Game bottom-right, Minimap top-right)
        zones.push_back({4, midX, loadedMapWidth - 1, 0, midY - 1});               // D: Yellow (Game top-right, Minimap bottom-right)

        // ===== build tile array =====
        std::vector<int> worldMap(loadedMapWidth * loadedMapHeight, 0);
        for (int y = 0; y < loadedMapHeight; ++y)
        {
            for (int x = 0; x < loadedMapWidth; ++x)
            {
                char c = mapRows[y][x];
                int tileValue = 0;

                if (c == '0')
                    tileValue = 0; // Floor
                else if (c == '1')
                    tileValue = 1; // Wall
                else if (c == 'D')
                    tileValue = 10; // Door
                else if (c == 'T')
                    tileValue = 11; // Terminal
                else if (c == 'E')
                    tileValue = 12; // Exit
                else if (c == 'C')
                    tileValue = 13; // Control Room Door (old marker)
                else if (c == 'K')
                    tileValue = 20; // Control Room Door (NEW dedicated tile)
                else if (c == 'F')
                    tileValue = 15; // Green Access Card
                else if (c == 'G')
                    tileValue = 16; // Purple Access Card
                else if (c == 'H')
                    tileValue = 17; // Red Access Card
                else if (c == 'J')
                    tileValue = 18; // Yellow Access Card
                else
                    tileValue = c - '0'; // Fallback numeric

                worldMap[y * loadedMapWidth + x] = tileValue;
            }
        }

        auto world = CreateGameObject("World");
        auto map = world->AddComponent<TileMap>();
        map->LoadFromArray(worldMap.data(), loadedMapWidth, loadedMapHeight);
        auto renderer = world->AddComponent<TileMapRenderer>();
        renderer->wallColorNS = cfg.wallColorNS;
        renderer->wallColorEW = cfg.wallColorEW;
        renderer->ceilingColor = cfg.ceilingColor;
        renderer->floorColor = cfg.floorColor;

        renderer->SetCeilingSprite(AssetManager::GetSprite("Assets/Tilemap/Wallpaper_02_White_1.png"));
        renderer->SetFloorSprite(AssetManager::GetSprite("Assets/Tilemap/s2 Concrete Squares 2 Grey.png"));
        renderer->SetWallSprite(AssetManager::GetSprite("Assets/Tilemap/Concrete_01_White_1.png"));
        renderer->SetDoorSprite(AssetManager::GetSprite("Assets/Tilemap/school_sliding_door.png"));

        auto tileMapCollider = world->AddComponent<TileMapCollider2D>();
        tileMapCollider->minHeight = cfg.tileMapColliderMinHeight;
        tileMapCollider->maxHeight = cfg.tileMapColliderMaxHeight;

        // ===== player =====

        auto player = GameObject::Find("Player");
        playerTf = player->transform;
        playerViewPos = playerTf->position;
        playerViewRot = playerTf->rotation;

        playerCam = player->GetComponent<Camera>();
        playerController = player->GetComponent<FPSController>();
        playerData = player->GetComponent<Player>();
        // Load persisted high score (if present)
        if (playerData)
        {
            std::ifstream hsFile("Assets/Config/Game/highscore.json");
            if (hsFile.is_open())
            {
                Json::Value root;
                Json::CharReaderBuilder builder;
                JSONCPP_STRING errs;
                if (parseFromStream(builder, hsFile, &root, &errs))
                {
                    float saved = playerData->GetHighScore();
                    saved = root.get("highScore", saved).asFloat();
                    playerData->SetHighScore(saved);
                }
                hsFile.close();
            }
        }
        playerHUD = player->GetComponent<PlayerHUD>();
        playerHUD->SetTileMap(map);
        playerHUD->SetPlayerTransform(playerTf);
        playerHUD->SetZoneGetter([this](int tx, int ty)
                                 { return this->GetZoneId(tx, ty); });
        playerHUD->SetHeatmap(&heatmap); // Connect heatmap data
        pauseMenu = player->GetComponent<PauseMenu>();
        loseScreen = player->GetComponent<LoseScreen>();
        winScreen = player->GetComponent<WinScreen>();
        controlsMenu = player->GetComponent<ControlsMenu>();
        powerUpController = player->GetComponent<PlayerPowerUpController>();

        showControlsAtStart = true;
        // ===== ML init =====
        zoneMgr.Init(map, loadedMapWidth, loadedMapHeight);
        heatmap.Init(loadedMapWidth, loadedMapHeight);

        zoneGrid.assign(loadedMapWidth * loadedMapHeight, 0);
        for (int y = 0; y < loadedMapHeight; ++y)
        {
            for (int x = 0; x < loadedMapWidth; ++x)
            {
                int zid = GetZoneId(x, y);
                zoneGrid[y * loadedMapWidth + x] = (zid > 0) ? zid : 0;
            }
        }
        zoneMgr.SetZoneGrid(zoneGrid);

        // ===== CCTV  =====

        auto terminals = GameObject::FindGameObjectsWithTag("Terminal");

        for (auto &tObj : terminals)
        {
            auto terminal = tObj->GetComponent<CCTVTerminal>();
            terminal->playerHUD = playerHUD;
            terminal->spriteRenderer = tObj->GetComponent<SpriteRenderer>();
            terminal->allGuards = guards;

            playerHUD->SetTerminalPosition(terminal->controlledZone, tObj->transform->position);
            cctvTerminals.push_back(terminal);
        }

        auto cctvs = GameObject::FindGameObjectsWithTag("Sensor");
        for (auto &cctv : cctvs)
        {
            auto sensor = cctv->GetComponent<CCTVCamera>();
            sensor->tile = map;
            sensor->tf = playerTf;
            sensor->hud = playerHUD;
            sensor->spriteRenderer = cctv->GetComponent<SpriteRenderer>();
            sensor->minimap = playerHUD->GetMinimap();
            cctvSensors.push_back(sensor);
            playerHUD->AddCCTVTransform(cctv->transform);

            int camTx = (int)cctv->transform->position.x;
            int camTy = (int)cctv->transform->position.y;
            int camZone = GetZoneId(camTx, camTy);

            for (auto &terminal : cctvTerminals)
            {
                if (camZone == terminal->controlledZone)
                {
                    terminal->controlledCameras.push_back(sensor);
                }
            }
        }
        cctvCooldown.assign(cctvSensors.size(), 0.0f);

        // ===== Spawn Control Room Doors from map markers ('C' / tile 13) =====

        // 1. Initialize All Doors (Calculate Facing & FOV)
        auto doorObjs = GameObject::FindGameObjectsWithTag("Door");
        for (auto *doorGO : doorObjs)
        {
            auto *door = doorGO->GetComponent<Engine::Door>();
            doors.push_back(door);
            auto *doorRenderer = doorGO->GetComponent<Engine::DoorRenderer>();

            // Connect the dynamic FOV from the TileMapRenderer
            if (doorRenderer && renderer)
            {
                doorRenderer->fov = renderer->fov;
            }

            // Calculate facing dynamically based on the TileMap walls
            if (door && map)
            {
                bool wallNorth = map->IsWall(door->tileX, door->tileY + 1);
                bool wallSouth = map->IsWall(door->tileX, door->tileY - 1);
                door->facing = (wallNorth || wallSouth) ? Engine::DoorFacing::EastWest : Engine::DoorFacing::NorthSouth;
                door->state = Engine::DoorState::Closed;
            }

            playerHUD->AddDoor(door);
        }

        // 2. Initialize Exit Door (Custom Interaction Lambda)
        auto exitDoors = Engine::GameObject::FindGameObjectsWithTag("ExitDoor");
        for (auto *exitGO : exitDoors)
        {
            auto *door = exitGO->GetComponent<Engine::Door>();
            doors.push_back(door);

            auto *interactable = exitGO->GetComponent<Engine::Interactable>();

            if (door && interactable)
            {
                int totalBooks = (int)Game::PuzzleManager::allBooks.size();

                if (Game::PuzzleManager::booksCollected < totalBooks)
                {
                    interactable->actionText = "EXIT LOCKED: Collect all books";
                }

                interactable->onInteract = [door, totalBooks, interactable]()
                {
                    if (Game::PuzzleManager::booksCollected >= totalBooks)
                    {
                        door->Toggle();
                        interactable->actionText = "Close";
                        Game::PuzzleManager::OpenExitDoor();
                    }
                    else
                    {
                        int collected = Game::PuzzleManager::booksCollected;
                        Game::PuzzleManager::activeBookTitle = "DOOR LOCKED";
                        Game::PuzzleManager::activeBookText = std::to_string(collected) + "/" + std::to_string(totalBooks) + " books collected";
                        Game::PuzzleManager::popupTimer = 3.0f;
                    }
                };

                if (door && map)
                {
                    bool wallNorth = map->IsWall(door->tileX, door->tileY + 1);
                    bool wallSouth = map->IsWall(door->tileX, door->tileY - 1);
                    door->facing = (wallNorth || wallSouth) ? Engine::DoorFacing::EastWest : Engine::DoorFacing::NorthSouth;
                    door->state = Engine::DoorState::Closed;
                }
                door->state = Engine::DoorState::Closed;
            }

            playerHUD->AddDoor(door);
        }

        exitPos = exitDoors[0]->transform->position;

        // 3. Initialize Control Room Doors (Link Components & HUD)
        auto controlDoors = Engine::GameObject::FindGameObjectsWithTag("ControlDoor");
        for (auto *cdGO : controlDoors)
        {
            auto *controlDoor = cdGO->GetComponent<Game::ControlRoomDoor>();
            auto *door = cdGO->GetComponent<Engine::Door>();
            doors.push_back(door);

            auto *interactable = cdGO->GetComponent<Engine::Interactable>();

            if (controlDoor && door && interactable)
            {
                // Link pointers internally
                controlDoor->door = door;
                controlDoor->interactable = interactable;

                // Register with Player HUD for pathfinding
                if (playerHUD)
                {
                    playerHUD->SetControlRoomPosition(controlDoor->zoneId, Vector2(cdGO->transform->position.x, cdGO->transform->position.y));
                }

                if (door && map)
                {
                    bool wallNorth = map->IsWall(door->tileX, door->tileY + 1);
                    bool wallSouth = map->IsWall(door->tileX, door->tileY - 1);
                    door->facing = (wallNorth || wallSouth) ? Engine::DoorFacing::EastWest : Engine::DoorFacing::NorthSouth;
                    door->state = Engine::DoorState::Closed;
                }
                door->state = Engine::DoorState::Closed;
            }

            playerHUD->AddDoor(door);
        }

        // ===== spawn 4 guards (1 per zone) =====
        guards.clear();

        auto guardObjects = GameObject::FindGameObjectsWithTag("Guard");
        for (auto &g : guardObjects)
        {
            auto guard = g->GetComponent<SecurityGuard>();
            guard->tileMap = map;
            guard->playerTf = playerTf;
            guard->playerData = playerData;
            guard->zoneMgr = &zoneMgr;
            guard->heatmap = &heatmap;
            guard->SetPatrolZoneOverride(guard->homeZoneId);
            guard->SetTerminals(&cctvTerminals);

            g->GetComponent<SpriteSheetRenderer>()->Play("Patrol");

            if (playerHUD)
                playerHUD->AddEnemyTransform(g->transform);

            guards.push_back(guard);
        }

        // start with zone 1 priority
        prevChosenZoneId = 1;
        ApplyPriorityZone_AssistNearest(prevChosenZoneId);

        // ===== Spawn NPCs: 2 Bully + 2 Annoying per zone (16 total) =====

        npcs.clear();
        auto npcObjects = Engine::GameObject::FindGameObjectsWithTag("NPC");

        for (auto &npcGO : npcObjects)
        {
            auto npc = npcGO->GetComponent<Game::NPC>();
            if (!npc)
                continue;

            npc->tileMap = map;
            npc->playerTf = playerTf;
            npc->playerData = playerData;

            if (playerCam)
            {
                auto shake = playerCam->gameObject->GetComponent<Game::CameraShake>();
                if (!shake)
                {
                    shake = playerCam->gameObject->AddComponent<Game::CameraShake>();
                    shake->Initialize();
                }
                npc->camShake = shake;
            }

            npc->SetPickGoalInZone([this](int zId, int &outX, int &outY) -> bool
                                   { return zoneMgr.PickRandomWalkableInZone(zId, outX, outY); });

            // Start patrol animation
            auto sr = npcGO->GetComponent<Engine::SpriteSheetRenderer>();
            if (sr)
            {
                sr->Play("Patrol");
            }

            npcs.push_back(npc);
        }

        // Add terminal icons to minimap
        if (playerHUD && playerHUD->GetMinimap())
        {
            for (auto &tObj : terminals)
            {
                auto pos = tObj->transform->position;
                playerHUD->GetMinimap()->AddTerminalIcon(
                    pos,
                    AssetManager::GetSprite("Assets/Sprites/hill_top_smile.png"));
            }
        }

        /* CreateEndPanel();*/
        SwitchToPlayerCam();

        // Initialize Level Editor
        // AddDoorTransform(Engine::Transform * doorTransform).Initialize(this, map, playerCam, playerTf);

        Game::PuzzleManager::BookDataInit();
        Game::PuzzleManager::Init();

        Game::PowerUpManager::Init();

        // Create Bandit visualizer (hidden by default)
        {
            auto bvGO = CreateGameObject("BanditVisualizer");
            if (bvGO)
            {
                banditVisualizer = bvGO->AddComponent<Game::BanditVisualizer>();
                if (banditVisualizer)
                {
                    banditVisualizer->bandit = &bandit;
                    banditVisualizer->SetGuardsPointer(&guards);
                    // connect cctv scores + heatmap + zone grid so visualizer can display them
                    banditVisualizer->SetCCTVScorePointer(cctvScore);
                    banditVisualizer->SetHeatmapPointer(&heatmap);
                    banditVisualizer->SetZoneGridPointer(&zoneGrid);
                    banditVisualizer->SetSelectedZone(prevChosenZoneId);
                    banditVisualizer->SetVisible(false);
                }
            }
        }
    }

    /*! \brief Called after rendering to draw editor overlays. */
    inline void FPSScene::OnAfterRender()
    {
        // Render level editor overlay on top of everything
        // AddDoorTransform(Engine::Transform * doorTransform).Render();
    }

    /*! \brief Main per-frame update handling game logic, AI and HUD updates. */
    inline void FPSScene::OnUpdate()
    {
        const auto &cfg = GetFPSSceneConfig();

        // Toggle Level Editor
        // if (Input::GetKeyDown(KeyCode::F1))
        // {
        //     AddDoorTransform(Engine::Transform * doorTransform).ToggleEditor();
        // }

        // Update editor (only if active)
        // AddDoorTransform(Engine::Transform * doorTransform).Update();

        // If editor is active, skip game logic
        // if (AddDoorTransform(Engine::Transform * doorTransform).IsActive())
        // {
        //     return;
        // }

        if (showControlsAtStart && controlsMenu)
        {
            controlsMenu->Show();
            showControlsAtStart = false;
        }

        if (controlsMenu && controlsMenu->IsVisible())
        {
            return;
        }

        // Toggle ML debug summary (no flood)
        if (Input::GetKeyDown(KeyCode::M))
        {
            debugML = !debugML;
            mlSummaryTimer = 0.0f;
        }

        // Toggle bandit visualizer with F3
        if (Input::GetKeyDown(KeyCode::F3))
        {
            if (banditVisualizer)
            {
                banditVisualizer->ToggleVisibility();
                // When showing visualizer, unlock cursor so user can interact
                if (banditVisualizer)
                {
                    // panel visibility toggled inside ToggleVisibility(); check panel state indirectly
                    // We can't access m_panel here, so simply toggle cursor registration based on panel visible state
                    // Best-effort: register/unregister cursor using a simple toggle stored on banditVisualizer via visible state
                }
            }
        }

        if (Input::GetKeyDown(KeyCode::Alpha0))
        {
            SwitchToPlayerCam();
        }

        // Lose panel
        /*if (!endPanelShown && playerData) {
            if (playerData->IsCaught()) {
                SoundManager::PlaySFX("lose");
                ShowEndPanel("YOU GOT CAUGHT. GAME OVER", "Quit", []() {
                    SceneManager::LoadScene("FPSScene");
                    });
            }
        }*/

        if (!gameEnded && playerData && playerData->IsCaught())
        {
            gameEnded = true;
            if (loseScreen)
            {
                loseScreen->Show();
            }
            // Ensure guard chase loop is silenced when player is caught
            Engine::SoundManager::StopLoop("guard_chasing");
            return;
        }

        if (!playerData || !playerTf)
            return;
        if (playerData->GetState() != PlayerState::Normal)
            return;

        // Update game timer
        playerData->UpdateGameTime(Engine::Time::deltaTime);

        // Update HUD elements
        if (playerHUD)
        {
            // Check zone changes for entry notification
            int tx = (int)std::floor(playerTf->position.x);
            int ty = (int)std::floor(playerTf->position.y);
            int currentZone = GetZoneId(tx, ty);

            playerHUD->CheckZoneChange(currentZone);

            // Update stamina display
            playerHUD->UpdateStamina(playerData->GetStamina(), playerData->GetStaminaMax());

            // Update current time display
            playerHUD->UpdateCurrentTime(playerData->GetGameTime());

            // Update high score display
            playerHUD->UpdateHighScore(playerData->GetHighScore());

            // Find nearest guard for proximity warning and camera shake
            float nearestGuardDistance = cfg.distanceSentinel;
            for (auto *guard : guards)
            {
                if (!guard || !guard->transform)
                    continue;

                Vector2 guardPos(guard->transform->position.x, guard->transform->position.y);
                Vector2 playerPos(playerTf->position.x, playerTf->position.y);

                float dx = guardPos.x - playerPos.x;
                float dy = guardPos.y - playerPos.y;
                float distance = std::sqrt(dx * dx + dy * dy);

                if (distance < nearestGuardDistance)
                {
                    nearestGuardDistance = distance;
                }
            }

            // guard proximity handled by UpdateGuardProximityLoop (looped sound)

            float nearestCCTVDistance = cfg.distanceSentinel;
            for (auto *cctv : cctvSensors)
            {
                if (!cctv || !cctv->transform)
                    continue;

                Vector2 cctvPos(cctv->transform->position.x, cctv->transform->position.y);
                Vector2 playerPos(playerTf->position.x, playerTf->position.y);

                float dx = cctvPos.x - playerPos.x;
                float dy = cctvPos.y - playerPos.y;
                float distance = std::sqrt(dx * dx + dy * dy);

                if (distance < nearestCCTVDistance)
                {
                    nearestCCTVDistance = distance;
                }
            }

            float nearestTerminalDistance = cfg.distanceSentinel;
            for (auto *terminal : cctvTerminals)
            {
                if (!terminal || !terminal->transform)
                    continue;

                Vector2 terminalPos(terminal->transform->position.x, terminal->transform->position.y);
                Vector2 playerPos(playerTf->position.x, playerTf->position.y);

                float dx = terminalPos.x - playerPos.x;
                float dy = terminalPos.y - playerPos.y;
                float distance = std::sqrt(dx * dx + dy * dy);

                if (distance < nearestTerminalDistance)
                {
                    nearestTerminalDistance = distance;
                }
            }

            // NPC proximity: separate Bully vs Annoying
            float nearestAnnoyingDistance = cfg.distanceSentinel;
            int annoyingCountNearby = 0;
            float nearestBullyDistance = cfg.distanceSentinel;
            int bullyCountNearby = 0;
            const float npcNoticeRadius = cfg.npcNoticeRadius; // within this range we count them as 'nearby'
            for (auto *npc : npcs)
            {
                if (!npc || !npc->transform)
                    continue;
                Vector2 npcPos(npc->transform->position.x, npc->transform->position.y);
                Vector2 playerPos(playerTf->position.x, playerTf->position.y);
                float dx = npcPos.x - playerPos.x;
                float dy = npcPos.y - playerPos.y;
                float distance = std::sqrt(dx * dx + dy * dy);

                if (npc->type == NPCType::Annoying)
                {
                    if (distance < nearestAnnoyingDistance)
                        nearestAnnoyingDistance = distance;
                    if (distance <= npcNoticeRadius)
                        annoyingCountNearby++;
                }
                else if (npc->type == NPCType::Bully)
                {
                    if (distance < nearestBullyDistance)
                        nearestBullyDistance = distance;
                    if (distance <= npcNoticeRadius)
                        bullyCountNearby++;
                }
            }

            // Update guard proximity warning
            playerHUD->UpdateGuardProximity(nearestGuardDistance);
            // Update NPC approach indicators (Annoying + Bully)
            playerHUD->UpdateNPCApproach(annoyingCountNearby, nearestAnnoyingDistance, bullyCountNearby, nearestBullyDistance);
            // Annoying classmate audio disabled: no one-shot SFX played here.
            SoundManager::UpdateGuardProximityLoop(nearestGuardDistance);
            SoundManager::UpdateCCTVHumLoop(nearestCCTVDistance);
            SoundManager::UpdateTerminalBuzzLoop(nearestTerminalDistance);
            // NPC annoying-classmate audio disabled: do not start or stop any
            // annoying-classmate audio channels from the scene.

            // Apply camera shake based on guard proximity
            const float shakeDistance = cfg.guardShakeDistance;
            if (nearestGuardDistance < shakeDistance && playerCam)
            {
                auto cameraShake = playerCam->gameObject->GetComponent<CameraShake>();
                if (!cameraShake)
                {
                    cameraShake = playerCam->gameObject->AddComponent<CameraShake>();
                    cameraShake->Initialize();
                }

                // Intensity increases as guard gets closer
                float intensity = (1.0f - (nearestGuardDistance / shakeDistance)) * cfg.guardShakeIntensityScale;
                cameraShake->Shake(cfg.guardShakeDuration, intensity);
            }

            // Update alert level display
            int guardCount = 0;
            for (auto *g : guards)
            {
                if (g && (g->IsInChaseState() || g->IsInSearchState()))
                {
                    guardCount++;
                }
            }

            // Determine alert stage (1-4 based on escalation)
            int alertStage = 1;
            if (fullEscalationActive)
                alertStage = 4;
            else if (priorityZoneHotTimer > fullEscalationThreshold * cfg.escalationStage3Ratio)
                alertStage = 3;
            else if (priorityZoneHotTimer > fullEscalationThreshold * cfg.escalationStage2Ratio)
                alertStage = 2;
            // Play full escalation SFX immediately when escalation triggers
            static bool escalationSfxPlayed = false;
            if (fullEscalationActive && !escalationSfxPlayed)
            {
                SoundManager::PlaySFX("full_escalation");
                escalationSfxPlayed = true;
            }
            if (!fullEscalationActive)
                escalationSfxPlayed = false;

            playerHUD->UpdateAlertLevel(alertStage, priorityZoneId, guardCount);
        }

        // Heatmap update
        {
            int tx = (int)playerTf->position.x;
            int ty = (int)playerTf->position.y;

            heatmap.AddAt(tx, ty, heatAddPerSec * Engine::Time::deltaTime);

            float f = std::pow(heatDecayPerSec, Engine::Time::deltaTime);
            heatmap.Decay(f);
        }

        // CCTV spikes + notify guards (with cooldown)
        for (size_t i = 0; i < cctvSensors.size(); ++i)
        {
            auto *s = cctvSensors[i];
            if (!s)
                continue;

            if (cctvCooldown[i] > 0.0f)
                cctvCooldown[i] -= Engine::Time::deltaTime;

            if (s->hasLastSeen)
            {
                int tx = (int)s->lastSeenWorld.x;
                int ty = (int)s->lastSeenWorld.y;
                int zid = GetZoneId(tx, ty);

                if (zid >= 1 && zid <= 4)
                {

                    // ALWAYS show in visualizer (real-time log)
                    if (banditVisualizer)
                    {
                        banditVisualizer->SetCCTVStatus(
                            zid,
                            s->lastSeenWorld.x,
                            s->lastSeenWorld.y,
                            true);
                    }

                    // ONLY spike score if cooldown allows
                    if (cctvCooldown[i] <= 0.0f)
                    {
                        cctvScore[zid] += cctvSpikeAmount;

                        priorityZoneId = zid;
                        ApplyPriorityZone_AssistNearest(zid);

                        for (auto *g : guards)
                            if (g)
                                g->OnCCTVSpotted(s->lastSeenWorld);

                        cctvCooldown[i] = cctvSpikeCooldown;
                    }
                }
            }

            // clear every frame so it doesn't spam
            s->hasLastSeen = false;
        }

        // Bandit tick
        banditTimer += Engine::Time::deltaTime;
        if (banditTimer >= banditInterval)
        {
            banditTimer = 0.0f;
            BanditTick();
        }

        // Update visualizer selected zone and summary when bandit changes
        if (banditVisualizer)
        {
            banditVisualizer->SetSelectedZone(prevChosenZoneId);
        }

        // ML summary every 5 sec only when debug on
        if (debugML)
        {
            mlSummaryTimer += Engine::Time::deltaTime;
            if (mlSummaryTimer >= cfg.mlSummaryInterval)
            {
                mlSummaryTimer = 0.0f;
                PrintMLSummary();
            }
        }

        // Win check
        Vector2 p(playerTf->position.x, playerTf->position.y);
        float dx = exitPos.x - p.x;
        float dy = exitPos.y - p.y;

        if (!printedWin && (dx * dx + dy * dy) <= winRadius * winRadius)
        {
            printedWin = true;
            gameEnded = true;
            // Stop any chase loops when player wins/escapes
            SoundManager::StopLoop("guard_chasing");
            SoundManager::PlaySFX("win");

            // Update high score if this is a new record
            float currentTime = playerData->GetGameTime();
            if (currentTime < playerData->GetHighScore())
            {
                playerData->SetHighScore(currentTime);
            }

            // Persist high score to disk
            if (playerData)
            {
                Json::Value root;
                root["highScore"] = playerData->GetHighScore();
                std::ofstream ofs("Assets/Config/Game/highscore.json");
                if (ofs.is_open())
                {
                    Json::StreamWriterBuilder writer;
                    ofs << Json::writeString(writer, root);
                    ofs.close();
                }
            }

            playerData->Win();
            if (winScreen)
            {
                winScreen->Show();
            }
        }
    }

    /*! \brief Cleanup scene state and release pointers before unload. */
    inline void FPSScene::OnUnload()
    {
        // Disconnect guards from terminal list to avoid dangling pointers
        for (auto *g : guards)
        {
            if (g)
                g->SetTerminals(nullptr);
        }

        // Clear terminals: remove references to cameras, HUD and guards
        for (auto *t : cctvTerminals)
        {
            if (!t)
                continue;
            t->controlledCameras.clear();
            t->playerHUD = nullptr;
            t->spriteRenderer = nullptr;
            t->allGuards.clear();
            t->floatingIcon = nullptr;
        }

        // Clear sensors: null out external pointers so Update() won't dereference them
        for (auto *s : cctvSensors)
        {
            if (!s)
                continue;
            s->tf = nullptr;
            s->tile = nullptr;
            s->hud = nullptr;
            s->minimap = nullptr;
            s->spriteRenderer = nullptr;
        }

        // Clear all scene-owned lists to avoid keeping stale pointers across reload
        cctvSensors.clear();
        cctvTerminals.clear();
        cctvCooldown.clear();

        guards.clear();

        // Clear other references
        powerUps.clear();
        powerUpBaseZ.clear();

        playerHUD = nullptr;
        playerData = nullptr;
        playerController = nullptr;
        playerTf = nullptr;
        playerCam = nullptr;

        PuzzleManager::FreeManager();

        loseScreen = nullptr;
        winScreen = nullptr;
        controlsMenu = nullptr;
        showControlsAtStart = false;

        /*if (modalCanvas) {
            UI::UIManager::GetInstance()->DestroyCanvas(modalCanvas);
            modalCanvas = nullptr;
        }
        */

        // Clear bandit visualizer reference
        banditVisualizer = nullptr;

        AEEditor::ObjectManager::loadedObjects.clear();
        AEEditor::ObjectManager::selectedObject = nullptr;
        AEEditor::ObjectManager::draggingObject = nullptr;
    }

} // namespace Game
