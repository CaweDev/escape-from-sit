/*!
 * \file AlphaEngine2.5D\Source\Game\PowerUp\PowerUpManager.cpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief [AUTO] Added Doxygen file header
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
        float spawnScale = 0.6f;
        float spawnRotation = 0.0f;
        int sortingOrder = 100;
        float defaultSpawnZ = 0.1f;
        float minSpawnDistance = 6.0f;
        int spawnIdStart = 1;

        std::string speedBoostSpritePath = "Assets/Sprites/Speed.png";
        std::string infiniteStaminaSpritePath = "Assets/Sprites/InfStamina.png";
        std::string shieldSpritePath = "Assets/Sprites/Shield.png";
        std::string invisibilitySpritePath = "Assets/Sprites/Invis.png";

        // Legacy explicit entries (still supported as fallback)
        struct Entry
        {
            int id = 0;
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.1f;
            int type = 0;
            std::string name;
            float duration = 0.0f;
        };
        std::vector<Entry> entries;

        // New model: powerup definitions + spawn counts
        struct PowerUpDef
        {
            int type = 0;
            std::string name;
            float duration = 0.0f;
            int count = 0; // optional inline count
        };
        struct SpawnCount
        {
            int type = 0;
            int count = 0;
        };

        std::vector<PowerUpDef> powerups;
        std::vector<SpawnCount> spawnCounts;

        int FindSpawnCountOverride(int type) const
        {
            for (const auto& sc : spawnCounts)
            {
                if (sc.type == type) return sc.count;
            }
            return -1;
        }

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

            // Legacy entries
            entries.clear();
            const Json::Value& arr = j["entries"];
            if (arr.isArray())
            {
                for (const auto& it : arr)
                {
                    Entry e;
                    e.id = it.get("id", e.id).asInt();
                    e.x = it.get("x", e.x).asFloat();
                    e.y = it.get("y", e.y).asFloat();
                    e.z = it.get("z", defaultSpawnZ).asFloat();
                    e.type = it.get("type", e.type).asInt();
                    e.name = it.get("name", "PowerUp").asString();
                    e.duration = it.get("duration", e.duration).asFloat();
                    entries.push_back(e);
                }
            }

            // New powerup definitions
            powerups.clear();
            const Json::Value& defs = j["powerups"];
            if (defs.isArray())
            {
                for (const auto& it : defs)
                {
                    PowerUpDef d;
                    d.type = it.get("type", d.type).asInt();
                    d.name = it.get("name", "PowerUp").asString();
                    d.duration = it.get("duration", d.duration).asFloat();
                    d.count = it.get("count", d.count).asInt();
                    powerups.push_back(d);
                }
            }

            // New spawn_counts object: { "0": 4, "1": 2, ... }
            spawnCounts.clear();
            const Json::Value& scObj = j["spawn_counts"];
            if (scObj.isObject())
            {
                for (Json::ValueConstIterator it = scObj.begin(); it != scObj.end(); ++it)
                {
                    SpawnCount sc;
                    sc.type = std::atoi(it.name().c_str());
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

    struct PowerUpSeed
    {
        int id;
        Game::PowerUpType type;
        const char* name;
        float duration;
    };

    static int CountOpenNeighbors(Engine::TileMap* map, int x, int y)
    {
        int open = 0;
        if (map->IsWalkable((float)x + 1.5f, (float)y + 0.5f)) ++open;
        if (map->IsWalkable((float)x - 0.5f, (float)y + 0.5f)) ++open;
        if (map->IsWalkable((float)x + 0.5f, (float)y + 1.5f)) ++open;
        if (map->IsWalkable((float)x + 0.5f, (float)y - 0.5f)) ++open;
        return open;
    }

    static bool HasAdjacentWall(Engine::TileMap* map, int x, int y)
    {
        return map->IsWall(x + 1, y) || map->IsWall(x - 1, y) || map->IsWall(x, y + 1) || map->IsWall(x, y - 1);
    }

    static bool IsHallwayTile(Engine::TileMap* map, int x, int y)
    {
        if (!map || map->IsWall(x, y))
            return false;
        const int open = CountOpenNeighbors(map, x, y);
        return (open >= 2 && open <= 3) && HasAdjacentWall(map, x, y);
    }

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

    static float DistSq(const Engine::Vector3& a, const Engine::Vector3& b)
    {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

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

    static int RandIndexAERandFloat(int maxExclusive)
    {
        if (maxExclusive <= 1) return 0;
        int idx = static_cast<int>(AERandFloat() * static_cast<float>(maxExclusive));
        if (idx < 0) idx = 0;
        if (idx >= maxExclusive) idx = maxExclusive - 1;
        return idx;
    }

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

        auto powerUpObj = scene->CreateGameObject("PowerUp_" + data.name);
        powerUpObj->transform->position = data.position;
        powerUpObj->transform->rotation = cfg.spawnRotation;
        powerUpObj->transform->localScale = Engine::Vector2(cfg.spawnScale, cfg.spawnScale);

        auto rb = powerUpObj->AddComponent<Engine::Rigidbody2D>();
        rb->bodyType = Engine::RigidbodyType2D::Static;

        auto sr = powerUpObj->AddComponent<Engine::SpriteRenderer>();
        sr->SetSprite(Engine::AssetManager::GetSprite(GetSpritePathForType(data.type).c_str()));

        sr->billboard = true;
        sr->enabled = true;
        sr->sortingOrder = cfg.sortingOrder;

        powerUpObj->AddComponent<Engine::Interactable>();

        auto logicComp = powerUpObj->AddComponent<PowerUpObject>();
        logicComp->reference = data;
    }

    void PowerUpManager::PowerUpDataInit() {
        allPowerUps.clear();
        const auto& cfg = GetPowerUpManagerConfig();

        std::vector<PowerUpSeed> seeds;

        // 1) NEW JSON MODEL (powerups + spawn_counts)
        if (!cfg.powerups.empty())
        {
            int nextId = cfg.spawnIdStart;
            for (const auto& d : cfg.powerups)
            {
                int count = d.count;
                const int overrideCount = cfg.FindSpawnCountOverride(d.type);
                if (overrideCount >= 0) count = overrideCount;

                if (count <= 0) continue;

                for (int i = 0; i < count; ++i)
                {
                    PowerUpSeed s;
                    s.id = nextId++;
                    s.type = static_cast<PowerUpType>(d.type);
                    s.name = d.name.empty() ? "PowerUp" : d.name.c_str();
                    s.duration = d.duration;
                    seeds.push_back(s);
                }
            }
        }
        // 2) LEGACY entries model (id/type/name/duration from entries)
        else if (!cfg.entries.empty())
        {
            seeds.reserve(cfg.entries.size());
            for (const auto& e : cfg.entries)
            {
                PowerUpSeed s;
                s.id = e.id;
                s.type = static_cast<PowerUpType>(e.type);
                s.name = e.name.empty() ? "PowerUp" : e.name.c_str();
                s.duration = e.duration;
                seeds.push_back(s);
            }
        }
        // 3) hard fallback seeds
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

        // Last fallback random pool: legacy entry positions (randomized, not fixed order)
        if (spawnPoints.empty() && !cfg.entries.empty())
        {
            for (const auto& e : cfg.entries)
            {
                spawnPoints.push_back({ e.x, e.y, e.z });
            }
        }

        // No valid random pool at all
        if (spawnPoints.empty())
        {
            return;
        }

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
                    const int idx = RandIndexAERandFloat(static_cast<int>(pool.size()));
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
                const int idx = RandIndexAERandFloat(static_cast<int>(pool.size()));
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
        PowerUpDataInit();
        for (const auto& data : allPowerUps) {
            CreatePowerUpObject(data);
        }
    }

    void PowerUpManager::FreeManager() {
        allPowerUps.clear();
    }

}
