/*!
 * \file AlphaEngine2.5D\Source\Game\PowerUp\PowerUp.cpp
 * \brief PowerUpObject component that represents a collectible power-up in the world, with floating and respawn behavior
 */

#include "PowerUp.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Interaction/Interactable.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Audio/SoundManager.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <iostream>
#include <cmath>

namespace
{
    struct PowerUpObjectConfig : public Engine::IConfig
    {
        float floatHeight = 0.15f;
        float floatSpeed = 2.0f;
        float rotationSpeed = 30.0f;
        float interactDistance = 2.5f;

        Engine::Color speedBoostHighlight = Engine::Color(0.0f, 0.5f, 0.5f, 0.0f);
        Engine::Color infiniteStaminaHighlight = Engine::Color(0.8f, 0.4f, 0.0f, 0.0f);
        Engine::Color shieldHighlight = Engine::Color(0.0f, 0.0f, 0.8f, 0.0f);
        Engine::Color invisibilityHighlight = Engine::Color(0.03f, 0.05f, 0.10f, 0.0f);
        Engine::Color defaultHighlight = Engine::Color(0.3f, 0.3f, 0.3f, 0.0f);

        static Engine::Color ParseColor(const Json::Value& j, const char* key, const Engine::Color& fallback)
        {
            if (!j.isMember(key) || !j[key].isObject()) return fallback;
            const auto& c = j[key];
            return Engine::Color(
                c.get("r", fallback.r).asFloat(),
                c.get("g", fallback.g).asFloat(),
                c.get("b", fallback.b).asFloat(),
                c.get("a", fallback.a).asFloat());
        }

        void FromJson(const Json::Value& j) override
        {
            floatHeight = j.get("floatHeight", floatHeight).asFloat();
            floatSpeed = j.get("floatSpeed", floatSpeed).asFloat();
            rotationSpeed = j.get("rotationSpeed", rotationSpeed).asFloat();
            interactDistance = j.get("interactDistance", interactDistance).asFloat();

            speedBoostHighlight = ParseColor(j, "speedBoostHighlight", speedBoostHighlight);
            infiniteStaminaHighlight = ParseColor(j, "infiniteStaminaHighlight", infiniteStaminaHighlight);
            shieldHighlight = ParseColor(j, "shieldHighlight", shieldHighlight);
            invisibilityHighlight = ParseColor(j, "invisibilityHighlight", invisibilityHighlight);
            defaultHighlight = ParseColor(j, "defaultHighlight", defaultHighlight);
        }
    };

    PowerUpObjectConfig g_powerUpObjectConfig;
    bool g_powerUpObjectConfigLoaded = false;

    const PowerUpObjectConfig& GetPowerUpObjectConfig()
    {
        if (!g_powerUpObjectConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/powerup_object.json", g_powerUpObjectConfig);
            if (g_powerUpObjectConfig.floatHeight < 0.0f) g_powerUpObjectConfig.floatHeight = 0.0f;
            if (g_powerUpObjectConfig.floatSpeed < 0.0f) g_powerUpObjectConfig.floatSpeed = 0.0f;
            if (g_powerUpObjectConfig.interactDistance < 0.1f) g_powerUpObjectConfig.interactDistance = 0.1f;
            g_powerUpObjectConfigLoaded = true;
        }
        return g_powerUpObjectConfig;
    }
}

namespace Game {

    /*!
     * \brief [AUTO] Initialize
     */
    void PowerUpObject::Initialize() {
        const auto& cfg = GetPowerUpObjectConfig();
        floatHeight = cfg.floatHeight;
        floatSpeed = cfg.floatSpeed;

        spriteRenderer = gameObject->GetComponent<Engine::SpriteRenderer>();
        interactable = gameObject->GetComponent<Engine::Interactable>();

        baseZ = gameObject->transform->position.z;

        if (spriteRenderer) {
            spriteRenderer->highlightColor = GetHighlightColor();
        }

        SetupInteraction();
        SetPickupEnabled(true);
    }

    /*!
     * \brief [AUTO] Update
     */
    void PowerUpObject::Update() {
        const auto& cfg = GetPowerUpObjectConfig();
        if (isCollected) {
            if (!allowRespawn) return;

            respawnTimer -= Engine::Time::deltaTime;
            if (respawnTimer <= 0.0f) {
                Respawn();
            }
            return;
        }

        if (gameObject->transform) {
            floatTime += Engine::Time::deltaTime * floatSpeed;
            gameObject->transform->position.z = baseZ + std::sin(floatTime) * floatHeight;
            gameObject->transform->rotation += Engine::Time::deltaTime * cfg.rotationSpeed;
        }
    }

    /*!
     * \brief [AUTO] SetupInteraction
     */
    void PowerUpObject::SetupInteraction() {
        if (!interactable) return;

        interactable->actionText = ("Collect " + reference.name);
        interactable->maxDistance = GetPowerUpObjectConfig().interactDistance;

        interactable->onInteract = [this]() {
            this->OnCollect();
        };
    }

    /*!
     * \brief [AUTO] OnCollect
     */
    void PowerUpObject::OnCollect() {
        if (isCollected) return;

        auto playerObj = Engine::GameObject::Find("Player");
        if (!playerObj) return;

        auto controller = playerObj->GetComponent<PlayerPowerUpController>();
        if (!controller) return;

        Engine::SoundManager::PlaySFX("powerup_collect");

        //std::cout << "[POWERUP] Got " << reference.name << "!\n";
        controller->AddPowerUp(reference.type, reference.duration);

        isCollected = true;
        respawnTimer = respawnDelay;
        SetPickupEnabled(false);
    }

    /*!
     * \brief [AUTO] SetPickupEnabled
     * \param isEnabled [in]
     */
    /*!
     * \brief [AUTO] SetPickupEnabled
     * \param isEnabled [in]
     */
    /*!
     * \brief [AUTO] SetPickupEnabled
     * \param isEnabled [in]
     */
    /*!
     * \brief [AUTO] SetPickupEnabled
     * \param isEnabled [in]
     */
    /*!
     * \brief [AUTO] SetPickupEnabled
     * \param isEnabled [in]
     */
    void PowerUpObject::SetPickupEnabled(bool isEnabled) {
        if (spriteRenderer) {
            spriteRenderer->enabled = isEnabled;
        }

        if (interactable) {
            interactable->enabled = isEnabled;
            if (isEnabled) {
                interactable->actionText = ("Collect " + reference.name);
            }
        }
    }

    /*!
     * \brief [AUTO] Respawn
     */
    void PowerUpObject::Respawn() {
        isCollected = false;
        respawnTimer = 0.0f;

        SetPickupEnabled(true);

        // reset bobbing baseline in case transform moved
        if (gameObject && gameObject->transform) {
            baseZ = gameObject->transform->position.z;
        }

        //std::cout << "[POWERUP] Respawned " << reference.name << ".\n";
    }

    /*!
     * \brief [AUTO] GetHighlightColor
     * \return 
     */
    /*!
     * \brief [AUTO] GetHighlightColor
     * \return 
     */
    /*!
     * \brief [AUTO] GetHighlightColor
     * \return 
     */
    /*!
     * \brief [AUTO] GetHighlightColor
     * \return 
     */
    /*!
     * \brief [AUTO] GetHighlightColor
     * \return 
     */
    Engine::Color PowerUpObject::GetHighlightColor() const {
        const auto& cfg = GetPowerUpObjectConfig();
		// set highlight color based on power-up type, with a default fallback
        switch (reference.type) {
        case PowerUpType::SpeedBoost:      return cfg.speedBoostHighlight;
        case PowerUpType::InfiniteStamina: return cfg.infiniteStaminaHighlight;
        case PowerUpType::Shield:          return cfg.shieldHighlight;
        case PowerUpType::Invisibility:    return cfg.invisibilityHighlight;
        default: return cfg.defaultHighlight;
        }
    }

} // namespace Game
