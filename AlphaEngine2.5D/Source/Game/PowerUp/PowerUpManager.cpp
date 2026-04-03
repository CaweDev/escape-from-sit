/*!
 * \file AlphaEngine2.5D\Source\Game\PowerUp\PowerUpManager.cpp
 * \brief PowerUpObject component that represents a collectible power-up in the world, with floating and respawn behavior
 */

#include "PowerUpManager.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Interaction/Interactable.hpp"
#include "Engine/Core/AssetManager.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include "AEEngine.h"
#include <algorithm>

namespace
{
    struct PowerUpManagerConfig : public Engine::IConfig
    {
		// General spawn settings (can be overridden in config, but defaults to these)
        float spawnScale = 0.6f;    
        float spawnRotation = 0.0f;
        int sortingOrder = 100;
        float defaultSpawnZ = 0.1f;
        float minSpawnDistance = 6.0f;
        int spawnIdStart = 1;

		// asset paths for sprites (can be overridden in config, but defaults to these)
        std::string speedBoostSpritePath = "Assets/Sprites/Speed.png";
        std::string infiniteStaminaSpritePath = "Assets/Sprites/InfStamina.png";
        std::string shieldSpritePath = "Assets/Sprites/Shield.png";
        std::string invisibilitySpritePath = "Assets/Sprites/Invis.png";

		// Power-up definitions (can be overridden in config, but defaults to these)
        struct PowerUpDef
        {
            Game::PowerUpType type = Game::PowerUpType::SpeedBoost;
            std::string name;
            float duration = 0.0f;
        };

		// Spawn count overrides for specific power-up types (can be overridden in config)
        struct SpawnCount
        {
            Game::PowerUpType type = Game::PowerUpType::SpeedBoost;
            int count = 0;
        };

        std::vector<PowerUpDef> powerups;
        std::vector<SpawnCount> spawnCounts;

		// search spawn count for a specific power-up type, returns 0 if not found
        int GetSpawnCount(Game::PowerUpType type) const
        {
            for (const auto& sc : spawnCounts)
            {
                if (sc.type == type) return sc.count;
            }
            return 0;
        }

		// Load configuration from JSON
        void FromJson(const Json::Value& j) override
        {
            spawnScale = j.get("spawnScale", spawnScale).asFloat();
            spawnRotation = j.get("spawnRotation", spawnRotation).asFloat();
            sortingOrder = j.get("sortingOrder", sortingOrder).asInt();
            defaultSpawnZ = j.get("defaultSpawnZ", defaultSpawnZ).asFloat();
            minSpawnDistance = j.get("minSpawnDistance", minSpawnDistance).asFloat();
            spawnIdStart = j.get("spawnIdStart", spawnIdStart).asInt();

            speedBoostSpritePath = j.get("speedBoostSpritePath", speedBoostSpritePath).asString();
            infiniteStaminaSpritePath = j.get("infiniteStaminaSpritePath", infiniteStaminaSpritePath).asString();
            shieldSpritePath = j.get("shieldSpritePath", shieldSpritePath).asString();
            invisibilitySpritePath = j.get("invisibilitySpritePath", invisibilitySpritePath).asString();

            powerups.clear();
            const Json::Value& defs = j["powerups"];
            if (defs.isArray())
            {
                for (const auto& it : defs)
                {
                    PowerUpDef d;
                    d.type = static_cast<Game::PowerUpType>(it.get("type", static_cast<int>(d.type)).asInt());
                    d.name = it.get("name", "PowerUp").asString();
                    d.duration = it.get("duration", d.duration).asFloat();
                    powerups.push_back(d);
                }
            }

            spawnCounts.clear();
            const Json::Value& scObj = j["spawn_counts"];
            if (scObj.isObject())
            {
                for (Json::ValueConstIterator it = scObj.begin(); it != scObj.end(); ++it)
                {
                    SpawnCount sc;
                    sc.type = static_cast<Game::PowerUpType>(std::atoi(it.name().c_str()));
                    sc.count = (*it).asInt();
                    spawnCounts.push_back(sc);
                }
            }
        }
    };

    PowerUpManagerConfig g_powerUpManagerConfig;
    bool g_powerUpManagerConfigLoaded = false;

    const PowerUpManagerConfig& GetPowerUpManagerConfig()
    {
        if (!g_powerUpManagerConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/powerup_manager.json", g_powerUpManagerConfig);
            g_powerUpManagerConfigLoaded = true;
        }
        return g_powerUpManagerConfig;
    }

    std::string GetSpritePathForType(Game::PowerUpType type)
    {
        const auto& cfg = GetPowerUpManagerConfig();
        switch (type) {
        case Game::PowerUpType::SpeedBoost:      return cfg.speedBoostSpritePath;
        case Game::PowerUpType::InfiniteStamina: return cfg.infiniteStaminaSpritePath;
        case Game::PowerUpType::Shield:          return cfg.shieldSpritePath;
        case Game::PowerUpType::Invisibility:    return cfg.invisibilitySpritePath;
        default:                           return cfg.speedBoostSpritePath;
        }
    }

	// struct to hold initial power-up data from config before spawning instances (position will be assigned later)
    struct PowerUpSeed
    {
        int id;
        Game::PowerUpType type;
        const char* name;
        float duration;
    };

	// Helper functions for collecting potential spawn points on the map
    static int CountOpenNeighbors(Engine::TileMap* map, int x, int y)
    {
        int open = 0;
        if (map->IsWalkable((float)x + 1.5f, (float)y + 0.5f)) ++open;
        if (map->IsWalkable((float)x - 0.5f, (float)y + 0.5f)) ++open;
        if (map->IsWalkable((float)x + 0.5f, (float)y + 1.5f)) ++open;
        if (map->IsWalkable((float)x + 0.5f, (float)y - 0.5f)) ++open;
        return open;
    }

	// Check if the tile at (x, y) has at least one adjacent wall tile (for better spawn placement)
    static bool HasAdjacentWall(Engine::TileMap* map, int x, int y)
    {
        return map->IsWall(x + 1, y) || map->IsWall(x - 1, y) || map->IsWall(x, y + 1) || map->IsWall(x, y - 1);
    }

	// Determine if a tile is a good candidate for hallway spawning (not a wall, has 2-3 open neighbors, and at least one adjacent wall)
    static bool IsHallwayTile(Engine::TileMap* map, int x, int y)
    {
        if (!map || map->IsWall(x, y))
            return false;
        const int open = CountOpenNeighbors(map, x, y);
        return (open >= 2 && open <= 3) && HasAdjacentWall(map, x, y);
    }

	// Collect potential spawn points in hallways (tiles that are not walls, have 2-3 open neighbors, and at least one adjacent wall)
    // and return their world positions
    static std::vector<Engine::Vector3> CollectHallwayPoints(Engine::TileMap* map, float z)
    {
        std::vector<Engine::Vector3> points;
        if (!map) return points;

        for (int y = 1; y < map->height - 1; ++y)
        {
            for (int x = 1; x < map->width - 1; ++x)
            {
                if (IsHallwayTile(map, x, y))
                {
                    points.push_back({ (float)x + 0.5f, (float)y + 0.5f, z });
                }
            }
        }
        return points;
    }

	// Calculate squared distance between two points (ignoring Z) for efficient distance checks
    static float DistSq(const Engine::Vector3& a, const Engine::Vector3& b)
    {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        return dx * dx + dy * dy;
    }


	// Check if a candidate spawn point is at least minDist away from all already selected points (to prevent clustering)
    static bool IsFarEnough(const Engine::Vector3& candidate, const std::vector<Engine::Vector3>& selected, float minDist)
    {
        const float minDistSq = minDist * minDist;
        for (const auto& p : selected)
        {
            if (DistSq(candidate, p) < minDistSq)
                return false;
        }
        return true;
    }

	// Get a random index in the range [0, maxExclusive) using AERandFloat, with clamping to ensure it's within bounds
    static int RandIndex(int maxExclusive)
    {
        if (maxExclusive <= 1) return 0;
        int idx = static_cast<int>(AERandFloat() * static_cast<float>(maxExclusive));
        if (idx < 0) idx = 0;
        if (idx >= maxExclusive) idx = maxExclusive - 1;
        return idx;
    }

	// Collect potential spawn points in hallways (tiles that are not walls, have 2-3 open neighbors, and at least one adjacent wall)
    // and return their world positions
    static std::vector<Engine::Vector3> CollectWalkablePoints(Engine::TileMap* map, float z)
    {
        std::vector<Engine::Vector3> points;
        if (!map) return points;

        for (int y = 1; y < map->height - 1; ++y)
        {
            for (int x = 1; x < map->width - 1; ++x)
            {
                if (map->IsWalkable((float)x + 0.5f, (float)y + 0.5f))
                {
                    points.push_back({ (float)x + 0.5f, (float)y + 0.5f, z });
                }
            }
        }
        return points;
    }
}

namespace Game {

    std::vector<PowerUpData> PowerUpManager::allPowerUps = {};

    void PowerUpManager::CreatePowerUpObject(PowerUpData data) {
        const auto& cfg = GetPowerUpManagerConfig();
        auto scene = Engine::SceneManager::GetActiveScene();
        if (!scene) return;

		// Create a new GameObject for the power-up and set its transform based on the provided data and config
        auto powerUpObj = scene->CreateGameObject("PowerUp_" + data.name);
        powerUpObj->transform->position = data.position;
        powerUpObj->transform->rotation = cfg.spawnRotation;
        powerUpObj->transform->localScale = Engine::Vector2(cfg.spawnScale, cfg.spawnScale);

		// Add a static Rigidbody2D to the power-up object (not affected by physics, just for potential collision detection)
        auto rb = powerUpObj->AddComponent<Engine::Rigidbody2D>();
        rb->bodyType = Engine::RigidbodyType2D::Static;

		// Add a SpriteRenderer component and set its sprite based on the power-up type, using the asset manager to load the sprite from the configured path
        auto sr = powerUpObj->AddComponent<Engine::SpriteRenderer>();
        sr->SetSprite(Engine::AssetManager::GetSprite(GetSpritePathForType(data.type).c_str()));

		// Set rendering properties for the sprite (billboard for 2.5D effect, sorting order from config)
        sr->billboard = true;
        sr->enabled = true;
        sr->sortingOrder = cfg.sortingOrder;

		// Add an Interactable component to allow the player to interact with the power-up (e.g., pick it up)
        powerUpObj->AddComponent<Engine::Interactable>();

		// Add the PowerUpObject component which contains the logic for floating animation, interaction, and respawning, 
        // and set its reference data to the provided power-up data
        auto logicComp = powerUpObj->AddComponent<PowerUpObject>();
        logicComp->reference = data;
    }

    void PowerUpManager::PowerUpDataInit() {
		// Clear any existing power-up data before initializing new ones
        allPowerUps.clear();
        const auto& cfg = GetPowerUpManagerConfig();

		// Create initial seeds based on config definitions, where each seed represents a power-up to be spawned
        // (with type, name, duration, and a unique ID)
        std::vector<PowerUpSeed> seeds;

        // (powerups + spawn_counts) 
        if (!cfg.powerups.empty())
        {
            int nextId = cfg.spawnIdStart;
            for (const auto& d : cfg.powerups)
            {
                const int count = cfg.GetSpawnCount(d.type);
                if (count <= 0) continue;

                for (int i = 0; i < count; ++i)
                {
                    PowerUpSeed s;
                    s.id = nextId++;
                    s.type = d.type;
                    s.name = d.name.empty() ? "PowerUp" : d.name.c_str();
                    s.duration = d.duration;
                    seeds.push_back(s);
                }
            }
        }

        // hard fallback seeds
        else
        {
            seeds = {
                { 4,  PowerUpType::InfiniteStamina, "Infinite Stamina", 15.0f },
                { 5,  PowerUpType::Invisibility,    "Invisibility",      8.0f },
                { 6,  PowerUpType::Shield,          "Shield",           20.0f },
                { 7,  PowerUpType::SpeedBoost,      "Speed Boost",      10.0f },
                { 8,  PowerUpType::Invisibility,    "Invisibility",      8.0f },
                { 9,  PowerUpType::InfiniteStamina, "Infinite Stamina", 15.0f },
                { 10, PowerUpType::SpeedBoost,      "Speed Boost",      10.0f },
                { 11, PowerUpType::Shield,          "Shield",           20.0f },
                { 12, PowerUpType::SpeedBoost,      "Speed Boost",      10.0f },
                { 13, PowerUpType::SpeedBoost,      "Speed Boost",      10.0f },
                { 14, PowerUpType::Invisibility,    "Invisibility",      8.0f }
            };
        }

		// fetch the tilemap to determine valid spawn locations for the power-ups, prioritizing hallway tiles for better placement, 
        // and falling back to any walkable tile if no hallway tiles are available
        Engine::TileMap* map = nullptr;
        if (auto* world = Engine::GameObject::Find("World"))
        {
            map = world->GetComponent<Engine::TileMap>();
        }

        // Primary random pool: hallways
        std::vector<Engine::Vector3> spawnPoints = CollectHallwayPoints(map, cfg.defaultSpawnZ);

        // Fallback random pool: any walkable tile
        if (spawnPoints.empty())
        {
            spawnPoints = CollectWalkablePoints(map, cfg.defaultSpawnZ);
        }


        // No valid random pool at all
        if (spawnPoints.empty())
        {
            return;
        }

		// For each seed, attempt to find a spawn point that is at least initialMinSpawnDistance away from all 
        // previously selected points to prevent clustering.
        constexpr float initialMinSpawnDistance = 6.0f;
        std::vector<Engine::Vector3> selectedPositions;
        selectedPositions.reserve(seeds.size());
        std::vector<Engine::Vector3> pool = spawnPoints;

        for (size_t i = 0; i < seeds.size(); ++i)
        {
            const auto& seed = seeds[i];
            bool found = false;
            Engine::Vector3 chosen = pool[0];
            float requiredDist = initialMinSpawnDistance;

            for (int relaxStep = 0; relaxStep < 8 && !found; ++relaxStep)
            {
                const int maxTries = static_cast<int>(pool.size()) * 3;
                for (int t = 0; t < maxTries; ++t)
                {
                    const int idx = RandIndex(static_cast<int>(pool.size()));
                    const Engine::Vector3 candidate = pool[idx];

                    if (IsFarEnough(candidate, selectedPositions, requiredDist))
                    {
                        chosen = candidate;
                        pool.erase(pool.begin() + idx);
                        found = true;
                        break;
                    }
                }
                requiredDist *= 0.85f;
            }

            if (!found)
            {
                const int idx = RandIndex(static_cast<int>(pool.size()));
                chosen = pool[idx];
                pool.erase(pool.begin() + idx);
            }

            selectedPositions.push_back(chosen);
            allPowerUps.push_back({ seed.id, chosen, seed.type, seed.name, seed.duration });

            if (pool.empty() && i + 1 < seeds.size())
            {
                pool = spawnPoints;
            }
        }
    }

    void PowerUpManager::Init() {
		// Initialize power-up data and spawn objects in the scene based on that data
        PowerUpDataInit();
        for (const auto& data : allPowerUps) {
            CreatePowerUpObject(data);
        }
    }

    void PowerUpManager::FreeManager() {
		// Clean up resources if needed (currently just clears the power-up data list, but could be expanded to destroy spawned objects if necessary)
        allPowerUps.clear();
    }

}
