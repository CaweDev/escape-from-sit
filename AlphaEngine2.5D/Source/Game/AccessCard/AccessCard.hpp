/**
 * @file AccessCard.hpp
 * @brief Access card collectible that unlocks control room doors
 */
#pragma once

#include "Engine/Core/Component.hpp"
#include "Engine/Graphics/Color.hpp"

namespace Engine {
    class SpriteRenderer;
    class Interactable;
}

namespace Game {

    /**
     * @enum AccessCardColor
     * @brief Color/type of access card (matches zone assignment)
     */
    enum class AccessCardColor {
        Green = 0,   // Zone B
        Purple = 1,  // Zone C  
        Red = 2,     // Zone D
        Yellow = 3   // Zone A
    };

    /**
     * @class AccessCard
     * @brief Collectible access card that unlocks control room doors
     */
    class AccessCard : public Engine::Component {
    public:
        /** Card color/type */
        AccessCardColor cardColor = AccessCardColor::Green;
        
        /** Card ID (0-3) */
        int cardId = 0;
        
        /** Floating animation */
        float floatTime = 0.0f;
        float baseZ = 0.5f;
        float floatHeight = 0.15f;
        float floatSpeed = 2.0f;
        // Prevent immediate pickup right after spawning (grace period)
        float spawnSafeDelay = 0.6f;
        float spawnSafeTimer = 0.0f;
        
        /** References */
        Engine::SpriteRenderer* spriteRenderer = nullptr;
        Engine::Interactable* interactable = nullptr;
        
        // Lifecycle
        void Initialize() override;
        void Update() override;
        
        /** Setup interaction for player pickup */
        void SetupInteraction();
        
        /** Handle card collection */
        void OnCollect();
        
        /** Get color name as string */
        const char* GetColorName() const;
        
        /** Get highlight color for this card type */
        Engine::Color GetHighlightColor() const;
    };

} // namespace Game
