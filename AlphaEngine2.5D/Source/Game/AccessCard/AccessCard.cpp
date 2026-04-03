/*!
 * \file AccessCard.cpp
 * \brief Implementation of AccessCard collectible
 */
#include "AccessCard.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Interaction/Interactable.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include "Game/Player/Player.hpp"
#include "Game/Scenes/FPSScene/UI/PlayerHUD.hpp"
#include "Engine/Audio/SoundManager.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <iostream>
#include <cmath>

namespace
{
    /*!
     * \struct AccessCardConfig
     * \brief JSON-driven configuration for AccessCard visuals and timing
     */
    struct AccessCardConfig : public Engine::IConfig
    {
        float floatHeight = 0.15f;
        float floatSpeed = 2.0f;
        float rotationSpeed = 30.0f;
        float spawnSafeDelay = 0.6f;

        Engine::Color greenHighlight = Engine::Color(0.0f, 0.5f, 0.0f, 0.0f);
        Engine::Color purpleHighlight = Engine::Color(0.5f, 0.0f, 0.5f, 0.0f);
        Engine::Color redHighlight = Engine::Color(0.5f, 0.0f, 0.0f, 0.0f);
        Engine::Color yellowHighlight = Engine::Color(0.5f, 0.5f, 0.0f, 0.0f);
        Engine::Color defaultHighlight = Engine::Color(0.3f, 0.3f, 0.3f, 0.0f);

        /*!
         * \brief Parse a color from JSON or return fallback
         * \param j [in] JSON root
         * \param key [in] Color key
         * \param fallback [in] Fallback color
         * \return Parsed color
         */
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

        /*!
         * \brief Load configuration from a JSON node
         * \param j [in] JSON node
         */
        void FromJson(const Json::Value& j) override
        {
            floatHeight = j.get("floatHeight", floatHeight).asFloat();
            floatSpeed = j.get("floatSpeed", floatSpeed).asFloat();
            rotationSpeed = j.get("rotationSpeed", rotationSpeed).asFloat();
            spawnSafeDelay = j.get("spawnSafeDelay", spawnSafeDelay).asFloat();

            greenHighlight = ParseColor(j, "greenHighlight", greenHighlight);
            purpleHighlight = ParseColor(j, "purpleHighlight", purpleHighlight);
            redHighlight = ParseColor(j, "redHighlight", redHighlight);
            yellowHighlight = ParseColor(j, "yellowHighlight", yellowHighlight);
            defaultHighlight = ParseColor(j, "defaultHighlight", defaultHighlight);
        }
    };

    AccessCardConfig g_accessCardConfig;
    bool g_accessCardConfigLoaded = false;

    /*!
     * \brief Retrieve the singleton AccessCard configuration
     * \return Reference to loaded AccessCardConfig
     */
    const AccessCardConfig& GetAccessCardConfig()
    {
        if (!g_accessCardConfigLoaded)
        {
            Engine::ConfigManager::ReadConfig("Assets/Config/Game/access_card.json", g_accessCardConfig);
            if (g_accessCardConfig.floatHeight < 0.0f) g_accessCardConfig.floatHeight = 0.0f;
            if (g_accessCardConfig.floatSpeed < 0.0f) g_accessCardConfig.floatSpeed = 0.0f;
            if (g_accessCardConfig.spawnSafeDelay < 0.0f) g_accessCardConfig.spawnSafeDelay = 0.0f;
            g_accessCardConfigLoaded = true;
        }
        return g_accessCardConfig;
    }
}

namespace Game {

/*!
 * \brief Component initialization: cache components and apply visual config
 */
void AccessCard::Initialize() {
    const auto& cfg = GetAccessCardConfig();
    floatHeight = cfg.floatHeight;
    floatSpeed = cfg.floatSpeed;
    spawnSafeDelay = cfg.spawnSafeDelay;

    // Cache component references
    spriteRenderer = gameObject->GetComponent<Engine::SpriteRenderer>();
    interactable = gameObject->GetComponent<Engine::Interactable>();
    
    // Store base Z position
    baseZ = gameObject->transform->position.z;
    
    // Set highlight color based on card type
    if (spriteRenderer) {
        spriteRenderer->highlightColor = GetHighlightColor();
    }
    
    // Setup interaction
    SetupInteraction();
}

/*!
 * \brief Per-frame update: floating animation, rotation, and spawn timer
 */
void AccessCard::Update() {
    const auto& cfg = GetAccessCardConfig();
    // Floating animation
    floatTime += Engine::Time::deltaTime * floatSpeed;
    gameObject->transform->position.z = baseZ + std::sin(floatTime) * floatHeight;
    
    // Gentle rotation
    gameObject->transform->rotation += Engine::Time::deltaTime * cfg.rotationSpeed;

    // Update spawn safe timer
    if (spawnSafeTimer < spawnSafeDelay) spawnSafeTimer += Engine::Time::deltaTime;
}

/*!
 * \brief Configure the Interactable callback with spawn-safe guard
 */
void AccessCard::SetupInteraction() {
    if (!interactable) return;

    // Set interaction callback with spawn-safety guard
    interactable->onInteract = [this]() {
        // ignore interactions during short spawn-safe period to avoid instant pickup
        if (spawnSafeTimer < spawnSafeDelay) {
            //std::cout << "[ACCESS CARD] Interaction ignored: spawn safe timer (" << spawnSafeTimer << ")\n";
            return;
        }
        this->OnCollect();
    };
}

/*!
 * \brief Handle collection: notify HUD, hide object and play SFX
 */
void AccessCard::OnCollect() {
    //std::cout << "[ACCESS CARD] Collected " << GetColorName() << " access card!\n";
    
    // Find player and notify HUD
    auto player = Engine::GameObject::Find("Player");
    if (player) {
        auto hud = player->GetComponent<PlayerHUD>();
        if (hud) {
            Engine::Vector2 cardPos(gameObject->transform->position.x, gameObject->transform->position.y);
            hud->CollectAccessCard(cardId, cardPos);
        }
    }
    
    // Hide the card instead of destroying it (like books do)
    // This allows it to respawn when the scene reloads
    gameObject->active = false;
    // Play collect SFX
    Engine::SoundManager::PlaySFX("card_collect");
}

/*!
 * \brief Get the textual name for the card color
 * \return C-string name of the card color
 */





const char* AccessCard::GetColorName() const {
    switch (cardColor) {
        case AccessCardColor::Green:  return "Green";
        case AccessCardColor::Purple: return "Purple";
        case AccessCardColor::Red:    return "Red";
        case AccessCardColor::Yellow: return "Yellow";
        default: return "Unknown";
    }
}

/*!
 * \brief Get the configured highlight color for this card type
 * \return Engine::Color value
 */





Engine::Color AccessCard::GetHighlightColor() const {
    const auto& cfg = GetAccessCardConfig();
    switch (cardColor) {
        case AccessCardColor::Green:  return cfg.greenHighlight;
        case AccessCardColor::Purple: return cfg.purpleHighlight;
        case AccessCardColor::Red:    return cfg.redHighlight;
        case AccessCardColor::Yellow: return cfg.yellowHighlight;
        default: return cfg.defaultHighlight;
    }
}

} // namespace Game
