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

namespace
{
    struct PowerUpManagerConfig : public Engine::IConfig
    {
        float spawnScale = 0.6f;
        float spawnRotation = 0.0f;
        int sortingOrder = 100;
        float defaultSpawnZ = 0.1f;

        std::string speedBoostSpritePath = "Assets/Sprites/Speed.png";
        std::string infiniteStaminaSpritePath = "Assets/Sprites/InfStamina.png";
        std::string shieldSpritePath = "Assets/Sprites/Shield.png";
        std::string invisibilitySpritePath = "Assets/Sprites/Invis.png";

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

        void FromJson(const Json::Value& j) override
        {
            spawnScale = j.get("spawnScale", spawnScale).asFloat();
            spawnRotation = j.get("spawnRotation", spawnRotation).asFloat();
            sortingOrder = j.get("sortingOrder", sortingOrder).asInt();
            defaultSpawnZ = j.get("defaultSpawnZ", defaultSpawnZ).asFloat();

            speedBoostSpritePath = j.get("speedBoostSpritePath", speedBoostSpritePath).asString();
            infiniteStaminaSpritePath = j.get("infiniteStaminaSpritePath", infiniteStaminaSpritePath).asString();
            shieldSpritePath = j.get("shieldSpritePath", shieldSpritePath).asString();
            invisibilitySpritePath = j.get("invisibilitySpritePath", invisibilitySpritePath).asString();

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
}

namespace Game {

    std::vector<PowerUpData> PowerUpManager::allPowerUps = {};

    /*!
     * \brief [AUTO] CreatePowerUpObject
     * \param data [in]
     */
    /*!
     * \brief [AUTO] CreatePowerUpObject
     * \param data [in]
     */
    /*!
     * \brief [AUTO] CreatePowerUpObject
     * \param data [in]
     */
    /*!
     * \brief [AUTO] CreatePowerUpObject
     * \param data [in]
     */
    /*!
     * \brief [AUTO] CreatePowerUpObject
     * \param data [in]
     */
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

        // Must add interactable before the Component so Awake gets it cleanly
        powerUpObj->AddComponent<Engine::Interactable>();

        auto logicComp = powerUpObj->AddComponent<PowerUpObject>();
        logicComp->reference = data;
    }

    /*!
     * \brief [AUTO] PowerUpDataInit
     */
    void PowerUpManager::PowerUpDataInit() {
        allPowerUps.clear();
        const auto& cfg = GetPowerUpManagerConfig();

        if (!cfg.entries.empty())
        {
            for (const auto& e : cfg.entries)
            {
                PowerUpData d;
                d.id = e.id;
                d.position = Engine::Vector3(e.x, e.y, e.z);
                d.type = static_cast<PowerUpType>(e.type);
                d.name = e.name;
                d.duration = e.duration;
                allPowerUps.push_back(d);
            }
            return;
        }

        // Hallways only fallback
        allPowerUps.push_back({ 4,  {18.5f,  8.5f, cfg.defaultSpawnZ}, PowerUpType::InfiniteStamina, "Infinite Stamina", 15.0f });
        allPowerUps.push_back({ 5,  {27.5f, 10.5f, cfg.defaultSpawnZ}, PowerUpType::Invisibility,    "Invisibility",      8.0f });
        allPowerUps.push_back({ 6,  {41.5f, 19.5f, cfg.defaultSpawnZ}, PowerUpType::Shield,          "Shield",           20.0f });
        allPowerUps.push_back({ 7,  {58.5f, 24.5f, cfg.defaultSpawnZ}, PowerUpType::SpeedBoost,      "Speed Boost",      10.0f });
        allPowerUps.push_back({ 8,  {72.5f, 27.5f, cfg.defaultSpawnZ}, PowerUpType::Invisibility,    "Invisibility",      8.0f });
        allPowerUps.push_back({ 9,  {86.5f, 31.5f, cfg.defaultSpawnZ}, PowerUpType::InfiniteStamina, "Infinite Stamina", 15.0f });
        allPowerUps.push_back({ 10, {103.5f, 36.5f, cfg.defaultSpawnZ}, PowerUpType::SpeedBoost,     "Speed Boost",      10.0f });
        allPowerUps.push_back({ 11, {116.5f, 39.5f, cfg.defaultSpawnZ}, PowerUpType::Shield,         "Shield",           20.0f });
        allPowerUps.push_back({ 12, {33.5f, 13.5f, cfg.defaultSpawnZ}, PowerUpType::SpeedBoost,      "Speed Boost",      10.0f });
        allPowerUps.push_back({ 13, {94.5f, 34.5f, cfg.defaultSpawnZ}, PowerUpType::SpeedBoost,      "Speed Boost",      10.0f });
        allPowerUps.push_back({ 14, {110.5f, 35.5f, cfg.defaultSpawnZ}, PowerUpType::Invisibility,   "Invisibility",      8.0f });
    }

    /*!
     * \brief [AUTO] Init
     */
    void PowerUpManager::Init() {
        PowerUpDataInit();
        for (const auto& data : allPowerUps) {
            CreatePowerUpObject(data);
        }
    }

    /*!
     * \brief [AUTO] FreeManager
     */
    void PowerUpManager::FreeManager() {
        allPowerUps.clear();
    }

}
