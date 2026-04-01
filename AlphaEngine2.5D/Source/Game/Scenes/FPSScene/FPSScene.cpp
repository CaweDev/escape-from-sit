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

        float spawnExcludeX = 2.5f;
        float spawnExcludeY = 2.5f;
        float spawnExcludeRadius = 7.0f;
        int spawnPickAttempts = 100;
        int spawnFallbackMaxRadius = 20;

        float bullyNPCSpeed = 2.0f;
        float bullyNPCTriggerRadius = 3.0f;
        float annoyingNPCSpeed = 1.2f;
        float annoyingNPCTriggerRadius = 2.5f;
        float annoyingNPCBlockTime = 1.5f;
        float spawnedNPCColliderRadius = 0.25f;
        float spawnedGuardColliderRadius = 0.3f;

        int cctvPerZone = 10;
        int cctvPlacementStep = 1;
        float cctvPlacementMinDist = 3.5f;
        float cctvPlacementZ = 0.85f;
        float cctvPlacementEpsilon = 0.02f;

        float cctvViewZ = 1.6f;
        float cctvScaleX = 0.25f;
        float cctvScaleY = 0.25f;
        int cctvSortingOrder = 300;
        float cctvDetectRadius = 3.0f;
        float cctvMaxRange = 6.0f;
        float cctvConeWidth = 70.0f;

        float terminalPosZ = 0.5f;
        float terminalScaleX = 0.5f;
        float terminalScaleY = 0.5f;
        int terminalSortingOrder = 250;

        float doorInteractMaxDistance = 2.5f;
        float accessCardInteractMaxDistance = 2.5f;


        float npcNoticeRadius = 6.0f;
        float guardShakeDistance = 6.0f;
        float guardShakeDuration = 0.1f;
        float guardShakeIntensityScale = 0.08f;
        float mlSummaryInterval = 5.0f;

        float tileMapColliderMinHeight = 0.0f;
        float tileMapColliderMaxHeight = 3.0f;
        float controlRoomDoorPosZ = 0.0f;
        float doorColliderMinHeight = 0.0f;
        float doorColliderMaxHeight = 2.0f;
        float cardSpawnDedupThreshold = 0.1f;
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

            spawnExcludeX = j.get("spawnExcludeX", spawnExcludeX).asFloat();
            spawnExcludeY = j.get("spawnExcludeY", spawnExcludeY).asFloat();
            spawnExcludeRadius = j.get("spawnExcludeRadius", spawnExcludeRadius).asFloat();
            spawnPickAttempts = j.get("spawnPickAttempts", spawnPickAttempts).asInt();
            spawnFallbackMaxRadius = j.get("spawnFallbackMaxRadius", spawnFallbackMaxRadius).asInt();

            bullyNPCSpeed = j.get("bullyNPCSpeed", bullyNPCSpeed).asFloat();
            bullyNPCTriggerRadius = j.get("bullyNPCTriggerRadius", bullyNPCTriggerRadius).asFloat();
            annoyingNPCSpeed = j.get("annoyingNPCSpeed", annoyingNPCSpeed).asFloat();
            annoyingNPCTriggerRadius = j.get("annoyingNPCTriggerRadius", annoyingNPCTriggerRadius).asFloat();
            annoyingNPCBlockTime = j.get("annoyingNPCBlockTime", annoyingNPCBlockTime).asFloat();
            spawnedNPCColliderRadius = j.get("spawnedNPCColliderRadius", spawnedNPCColliderRadius).asFloat();
            spawnedGuardColliderRadius = j.get("spawnedGuardColliderRadius", spawnedGuardColliderRadius).asFloat();


            cctvPerZone = j.get("cctvPerZone", cctvPerZone).asInt();
            cctvPlacementStep = j.get("cctvPlacementStep", cctvPlacementStep).asInt();
            cctvPlacementMinDist = j.get("cctvPlacementMinDist", cctvPlacementMinDist).asFloat();
            cctvPlacementZ = j.get("cctvPlacementZ", cctvPlacementZ).asFloat();
            cctvPlacementEpsilon = j.get("cctvPlacementEpsilon", cctvPlacementEpsilon).asFloat();

            cctvViewZ = j.get("cctvViewZ", cctvViewZ).asFloat();
            cctvScaleX = j.get("cctvScaleX", cctvScaleX).asFloat();
            cctvScaleY = j.get("cctvScaleY", cctvScaleY).asFloat();
            cctvSortingOrder = j.get("cctvSortingOrder", cctvSortingOrder).asInt();
            cctvDetectRadius = j.get("cctvDetectRadius", cctvDetectRadius).asFloat();
            cctvMaxRange = j.get("cctvMaxRange", cctvMaxRange).asFloat();
            cctvConeWidth = j.get("cctvConeWidth", cctvConeWidth).asFloat();

            terminalPosZ = j.get("terminalPosZ", terminalPosZ).asFloat();
            terminalScaleX = j.get("terminalScaleX", terminalScaleX).asFloat();
            terminalScaleY = j.get("terminalScaleY", terminalScaleY).asFloat();
            terminalSortingOrder = j.get("terminalSortingOrder", terminalSortingOrder).asInt();

            doorInteractMaxDistance = j.get("doorInteractMaxDistance", doorInteractMaxDistance).asFloat();
            accessCardInteractMaxDistance = j.get("accessCardInteractMaxDistance", accessCardInteractMaxDistance).asFloat();

            npcNoticeRadius = j.get("npcNoticeRadius", npcNoticeRadius).asFloat();
            guardShakeDistance = j.get("guardShakeDistance", guardShakeDistance).asFloat();
            guardShakeDuration = j.get("guardShakeDuration", guardShakeDuration).asFloat();
            guardShakeIntensityScale = j.get("guardShakeIntensityScale", guardShakeIntensityScale).asFloat();
            mlSummaryInterval = j.get("mlSummaryInterval", mlSummaryInterval).asFloat();

            tileMapColliderMinHeight = j.get("tileMapColliderMinHeight", tileMapColliderMinHeight).asFloat();
            tileMapColliderMaxHeight = j.get("tileMapColliderMaxHeight", tileMapColliderMaxHeight).asFloat();
            controlRoomDoorPosZ = j.get("controlRoomDoorPosZ", controlRoomDoorPosZ).asFloat();
            doorColliderMinHeight = j.get(" doorColliderMinHeight", doorColliderMinHeight).asFloat();
            doorColliderMaxHeight = j.get("doorColliderMaxHeight", doorColliderMaxHeight).asFloat();
            cardSpawnDedupThreshold = j.get("cardSpawnDedupThreshold", cardSpawnDedupThreshold).asFloat();
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

        spawnExcludeX = cfg.spawnExcludeX;
        spawnExcludeY = cfg.spawnExcludeY;
        spawnExcludeRadius = cfg.spawnExcludeRadius;
        spawnPickAttempts = cfg.spawnPickAttempts;
        spawnFallbackMaxRadius = cfg.spawnFallbackMaxRadius;

        bullyNPCSpeed = cfg.bullyNPCSpeed;
        bullyNPCTriggerRadius = cfg.bullyNPCTriggerRadius;
        annoyingNPCSpeed = cfg.annoyingNPCSpeed;
        annoyingNPCTriggerRadius = cfg.annoyingNPCTriggerRadius;
        annoyingNPCBlockTime = cfg.annoyingNPCBlockTime;
        spawnedNPCColliderRadius = cfg.spawnedNPCColliderRadius;
        spawnedGuardColliderRadius = cfg.spawnedGuardColliderRadius;

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

        // ===== Read exit position from map (fallback to default) =====
        auto mapExitPositions = map->GetExitPositions();
        if (!mapExitPositions.empty())
        {
            exitPos = mapExitPositions[0];
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

            g->GetComponent<SpriteSheetRenderer>()->Play("Patrol");

            if (playerHUD)
                playerHUD->AddEnemyTransform(g->transform);

            guards.push_back(guard);
        }

        // start with zone 1 priority
        prevChosenZoneId = 1;
        ApplyPriorityZone_AssistNearest(prevChosenZoneId);

        // ===== CCTV placement =====
        Sprite *cctvSprite = AssetManager::GetSprite("Assets/Sprites/cctv.png");

        auto placements = Game::GenerateCCTVsPerZone(
            map, zoneGrid, loadedMapWidth, loadedMapHeight,
            cfg.cctvPerZone,
            cfg.cctvPlacementStep,
            cfg.cctvPlacementMinDist,
            cfg.cctvPlacementZ,
            cfg.cctvPlacementEpsilon);

        for (int i = 0; i < (int)placements.size(); ++i)
        {
            auto &p = placements[i];

            Vector3 view = p.pos;
            view.z = cfg.cctvViewZ;
            cctvViewPos.push_back(view);
            cctvViewRot.push_back(p.rot);

            auto camObj = CreateGameObject("CCTV_" + std::to_string(i + 1));
            camObj->transform->position = p.pos;
            camObj->transform->rotation = p.rot;
            camObj->transform->localScale = Vector2(cfg.cctvScaleX, cfg.cctvScaleY);

            auto sr = camObj->AddComponent<SpriteRenderer>();
            sr->SetSprite(cctvSprite);
            sr->billboard = true;
            sr->sortingOrder = cfg.cctvSortingOrder;

            playerHUD->AddCCTVTransform(camObj->transform);

            auto sensor = camObj->AddComponent<CCTVCamera>();
            sensor->id = i + 1;
            sensor->tile = map;
            sensor->tf = playerTf;
            sensor->hud = playerHUD;
            sensor->minimap = playerHUD->GetMinimap();
            sensor->detectRadius = cfg.cctvDetectRadius;
            sensor->maxRange = cfg.cctvMaxRange;
            sensor->coneWidth = cfg.cctvConeWidth;
            sensor->needLOS = true;
            sensor->spriteRenderer = sr;

            cctvSensors.push_back(sensor);
        }

        cctvCooldown.assign(cctvSensors.size(), 0.0f);

        // ===== CCTV Terminals - Spawn from map markers ('T' in map.txt) =====
        Sprite *terminalSprite = AssetManager::GetSprite("Assets/Sprites/powerup3.png");

        // Get terminal positions from map
        auto terminalPositions = map->GetTerminalPositions();

        if (terminalPositions.empty())
        {
        }

        // Spawn terminals at marked positions
        for (size_t i = 0; i < terminalPositions.size(); ++i)
        {
            Vector2 pos = terminalPositions[i];

            // Determine zone from position (use floor to get tile coordinates)
            // Terminal positions are at tile center (x.5, y.5), so floor gives the tile
            int tileX = (int)std::floor(pos.x);
            int tileY = (int)std::floor(pos.y);
            int zoneId = GetZoneId(tileX, tileY);
            if (zoneId < 1 || zoneId > 4)
            {
                continue;
            }

            // Create terminal GameObject
            auto terminalObj = CreateGameObject("CCTVTerminal_Zone" + std::to_string(zoneId));
            terminalObj->transform->position = Vector3(pos.x, pos.y, cfg.terminalPosZ);
            terminalObj->transform->localScale = Vector2(cfg.terminalScaleX, cfg.terminalScaleY);

            auto sr = terminalObj->AddComponent<SpriteRenderer>();
            sr->SetSprite(terminalSprite);
            sr->billboard = true;
            sr->sortingOrder = cfg.terminalSortingOrder;

            auto terminal = terminalObj->AddComponent<CCTVTerminal>();
            terminal->controlledZone = zoneId;
            terminal->playerHUD = playerHUD;
            terminal->spriteRenderer = sr;
            terminal->allGuards = guards;

            // CCTVTerminal's SetupInteraction() is called in Awake()
            // It will automatically create and configure the Interactable component

            // Assign cameras to terminals based on zone
            for (size_t j = 0; j < cctvSensors.size(); ++j)
            {
                auto *cam = cctvSensors[j];
                if (!cam)
                    continue;

                // Determine camera's zone
                int camTx = (int)cam->transform->position.x;
                int camTy = (int)cam->transform->position.y;
                int camZone = GetZoneId(camTx, camTy);

                if (camZone == zoneId)
                {
                    terminal->controlledCameras.push_back(cam);
                }
            }

            cctvTerminals.push_back(terminal);
        }

        // Register terminal positions with PlayerHUD for pathfinding
        for (size_t i = 0; i < terminalPositions.size(); ++i)
        {
            Vector2 pos = terminalPositions[i];
            int tileX = (int)std::floor(pos.x);
            int tileY = (int)std::floor(pos.y);
            int zoneId = GetZoneId(tileX, tileY);
            if (zoneId >= 1 && zoneId <= 4)
            {
                playerHUD->SetTerminalPosition(zoneId, pos);
            }
        }

        // ===== Spawn Control Room Doors from map markers ('C' / tile 13) =====
        auto controlRoomDoorPositions = map->GetControlRoomDoorPositions();

        // Register control room door positions with PlayerHUD for pathfinding
        for (size_t i = 0; i < controlRoomDoorPositions.size(); ++i)
        {
            Vector2 pos = controlRoomDoorPositions[i];
            int tileX = (int)std::floor(pos.x);
            int tileY = (int)std::floor(pos.y);
            int zoneId = GetZoneId(tileX, tileY);
            if (zoneId >= 1 && zoneId <= 4)
            {
                playerHUD->SetControlRoomPosition(zoneId, pos);
            }
        }

        for (size_t i = 0; i < controlRoomDoorPositions.size(); ++i)
        {
            Vector2 pos = controlRoomDoorPositions[i];

            // Determine zone from position
            int tileX = (int)std::floor(pos.x);
            int tileY = (int)std::floor(pos.y);
            int zoneId = GetZoneId(tileX, tileY);
            if (zoneId < 1 || zoneId > 4)
            {
                continue;
            }

            // Create door GameObject
            std::string doorName = "ControlRoomDoor_Zone" + std::to_string(zoneId);
            auto doorGO = CreateGameObject(doorName);
            doorGO->transform->position = Vector3(pos.x, pos.y, cfg.controlRoomDoorPosZ);

            // Determine door facing based on surrounding walls
            bool wallNorth = map->IsWall(tileX, tileY + 1);
            bool wallSouth = map->IsWall(tileX, tileY - 1);
            Engine::DoorFacing facing = (wallNorth || wallSouth) ? Engine::DoorFacing::EastWest : Engine::DoorFacing::NorthSouth;

            // Add BoxCollider2D
            auto doorCollider = doorGO->AddComponent<Engine::BoxCollider2D>();
            doorCollider->minHeight = cfg.doorColliderMinHeight;
            doorCollider->maxHeight = cfg.doorColliderMaxHeight;
            doorCollider->isTrigger = false;

            // Add Door component
            auto door = doorGO->AddComponent<Engine::Door>();
            door->tileX = tileX;
            door->tileY = tileY;
            door->doorType = 10;
            door->facing = facing;
            door->collider = doorCollider;
            door->state = Engine::DoorState::Closed;

            // Add DoorRenderer
            auto doorRenderer = doorGO->AddComponent<Engine::DoorRenderer>();
            doorRenderer->fov = renderer->fov;
            doorRenderer->sprite = renderer->GetDoorSprite();

            // Add Interactable
            auto interactable = doorGO->AddComponent<Engine::Interactable>();
            interactable->actionText = "LOCKED - Access Cards Required (0/4)";
            interactable->maxDistance = cfg.doorInteractMaxDistance;

            // Add ControlRoomDoor component
            auto controlDoor = doorGO->AddComponent<Game::ControlRoomDoor>();
            controlDoor->zoneId = zoneId;
            controlDoor->door = door;
            controlDoor->interactable = interactable;
        }

        // Connect terminals to guards
        for (auto *guard : guards)
        {
            if (guard)
            {
                guard->SetTerminals(&cctvTerminals);
            }
        }

        // ===== Spawn NPCs: 2 Bully + 2 Annoying per zone (16 total) =====
        npcs.clear();
        for (int z = 1; z <= 4; ++z)
        {
            // Spawn with extra checks - avoid player spawn and doors
            SpawnNPCInZone(z, NPCType::Bully, map, "Assets/Sprites/bully.png");
            SpawnNPCInZone(z, NPCType::Annoying, map, "Assets/Sprites/annoying_classmate.png");
        }

        // Add terminal icons to minimap
        if (playerHUD && playerHUD->GetMinimap())
        {
            for (auto &pos : terminalPositions)
            {
                playerHUD->GetMinimap()->AddTerminalIcon(
                    pos,
                    AssetManager::GetSprite("Assets/Sprites/hill_top_smile.png"));
            }
        }

        /* CreateEndPanel();*/
        SwitchToPlayerCam();

        // Initialize Level Editor
        levelEditor.Initialize(this, map, playerCam, playerTf);

        Game::PuzzleManager::BookDataInit("Assets/Config/Game/books.json");
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
        levelEditor.Render();
    }

    /*! \brief Main per-frame update handling game logic, AI and HUD updates. */
    inline void FPSScene::OnUpdate()
    {
        const auto &cfg = GetFPSSceneConfig();

        // Toggle Level Editor
        if (Input::GetKeyDown(KeyCode::F1))
        {
            levelEditor.ToggleEditor();
        }

        // Update editor (only if active)
        levelEditor.Update();

        // If editor is active, skip game logic
        if (levelEditor.IsActive())
        {
            return;
        }

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

        // CCTV view switching (silent)
        int camCount = (int)cctvViewPos.size();
        if (camCount > 0)
        {
            /* if (Input::GetKeyDown(KeyCode::DownArrow)) {
                 if (activeCCTV < 0) activeCCTV = 0;
                 else activeCCTV = (activeCCTV + 1) % camCount;
                 SwitchToCCTV(activeCCTV);
             }
             if (Input::GetKeyDown(KeyCode::UpArrow)) {
                 if (activeCCTV < 0) activeCCTV = camCount - 1;
                 else activeCCTV = (activeCCTV - 1 + camCount) % camCount;
                 SwitchToCCTV(activeCCTV);
             }*/
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
        cctvViewPos.clear();
        cctvViewRot.clear();
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
