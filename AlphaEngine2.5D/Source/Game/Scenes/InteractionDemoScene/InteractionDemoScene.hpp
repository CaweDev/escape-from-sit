#pragma once

#include "Engine/Scene/Scene.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Graphics/TileMapRenderer.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Physics/CircleCollider2D.hpp"
#include "Engine/Physics/TileMapCollider2D.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Interaction/Interactable.hpp"
#include "Engine/Interaction/InteractionController.hpp"
#include "Engine/Core/Time.hpp"

#include "Game/FPSController.hpp"
#include "Game/Player.hpp"

#include <iostream>
#include <memory>
#include <cmath>

namespace Game {

using namespace Engine;

/**
 * @class InteractionDemoScene
 * @brief Demonstrates the Interactable system with test objects
 */
class InteractionDemoScene : public Scene {
public:
    InteractionDemoScene() : Scene("InteractionDemo") {}

    // Sprites (kept alive for duration of scene)
    std::shared_ptr<Sprite> doorSprite;
    std::shared_ptr<Sprite> keySprite;
    std::shared_ptr<Sprite> chestSprite;
    std::shared_ptr<Sprite> wallSprite;
    std::shared_ptr<Sprite> floorSprite;
    std::shared_ptr<Sprite> ceilingSprite;

    // References
    FPSController* playerController = nullptr;
    Transform* playerTf = nullptr;

    // Game state
    bool hasKey = false;
    bool doorOpen = false;
    bool chestLooted = false;

    void OnLoad() override {
        std::cout << "\n=== INTERACTION DEMO SCENE ===\n";
        std::cout << "Controls:\n";
        std::cout << "  WASD - Move\n";
        std::cout << "  Mouse - Look\n";
        std::cout << "  E - Interact (when looking at object)\n";
        std::cout << "==============================\n\n";

        // Simple map: 10x10 with a small room
        const int mapData[] = {
            1,1,1,1,1,1,1,1,1,1,
            1,0,0,0,0,0,0,0,0,1,
            1,0,0,0,0,0,0,0,0,1,
            1,0,0,0,1,1,0,0,0,1,
            1,0,0,0,1,0,0,0,0,1,
            1,0,0,0,0,0,0,0,0,1,
            1,0,0,0,0,0,0,0,0,1,
            1,0,0,0,0,0,0,0,0,1,
            1,0,0,0,0,0,0,0,0,1,
            1,1,1,1,1,1,1,1,1,1
        };

        // Create world with tilemap
        auto world = CreateGameObject("World");
        auto map = world->AddComponent<TileMap>();
        map->LoadFromArray(mapData, 10, 10);

        auto renderer = world->AddComponent<TileMapRenderer>();
        renderer->wallColorNS = Color(0.5f, 0.5f, 0.6f, 1.0f);
        renderer->wallColorEW = Color(0.4f, 0.4f, 0.5f, 1.0f);
        renderer->ceilingColor = Color(0.2f, 0.2f, 0.3f, 1.0f);
        renderer->floorColor = Color(0.3f, 0.3f, 0.35f, 1.0f);

        // Load sprites (using existing assets or fallback to colored sprites)
        renderer->SetWallSprite(AssetManager::GetSprite("Assets/Tilemap/modern_school_wall.png"));
        renderer->SetFloorSprite(AssetManager::GetSprite("Assets/Tilemap/modern_school_floor.png"));
        renderer->SetCeilingSprite(AssetManager::GetSprite("Assets/Tilemap/modern_school_ceiling.png"));

        auto tileMapCollider = world->AddComponent<TileMapCollider2D>();
        tileMapCollider->minHeight = 0.0f;
        tileMapCollider->maxHeight = 3.0f;

        // Create player with camera
        auto player = CreateGameObject("Player");
        auto cam = player->AddComponent<Camera>();
        
        auto rb = player->AddComponent<Rigidbody2D>();
        rb->gravityScale = 1.0f;
        rb->drag = 5.0f;

        auto col = player->AddComponent<CircleCollider2D>();
        col->radius = 0.2f;
        col->minHeight = 0.0f;
        col->maxHeight = 1.8f;

        playerController = player->AddComponent<FPSController>();
        player->AddComponent<Player>();
        playerTf = player->transform;

        playerTf->position = Vector3(2.5f, 2.5f, 0.0f);
        playerTf->rotation = 45.0f;

        // Add InteractionController to the player!
        auto interactionCtrl = player->AddComponent<InteractionController>();
        interactionCtrl->interactKey = KeyCode::E;

        // Load interactable object sprites
        // Using existing powerup sprites as placeholders
        doorSprite = std::make_shared<Sprite>();
        doorSprite->Load("Assets/powerup1.png");
        
        keySprite = std::make_shared<Sprite>();
        keySprite->Load("Assets/powerup2.png");
        
        chestSprite = std::make_shared<Sprite>();
        chestSprite->Load("Assets/powerup3.png");

        // === INTERACTABLE OBJECTS ===

        // 1. Key pickup
        auto keyObj = CreateGameObject("Key");
        keyObj->transform->position = Vector3(7.5f, 2.5f, 0.25f);
        keyObj->transform->localScale = Vector2(0.4f, 0.4f);

        auto keySR = keyObj->AddComponent<SpriteRenderer>();
        keySR->SetSprite(keySprite);
        keySR->billboard = true;
        keySR->sortingOrder = 100;
        keySR->highlightColor = Color(0.3f, 0.3f, 0.0f, 0.0f); // Yellow glow

        auto keyInteract = keyObj->AddComponent<Interactable>();
        keyInteract->actionText = "Pick up Key";
        keyInteract->maxDistance = 2.5f;
        keyInteract->onInteract = [this, keyObj]() {
            if (!hasKey) {
                hasKey = true;
                keyObj->active = false; // Hide the key
                std::cout << "[INTERACTION] You picked up the KEY!\n";
            }
        };

        // 2. Door (requires key)
        auto doorObj = CreateGameObject("Door");
        doorObj->transform->position = Vector3(4.5f, 4.5f, 0.5f);
        doorObj->transform->localScale = Vector2(0.6f, 1.0f);

        auto doorSR = doorObj->AddComponent<SpriteRenderer>();
        doorSR->SetSprite(doorSprite);
        doorSR->billboard = true;
        doorSR->sortingOrder = 100;
        doorSR->highlightColor = Color(0.0f, 0.3f, 0.3f, 0.0f); // Cyan glow

        auto doorInteract = doorObj->AddComponent<Interactable>();
        doorInteract->maxDistance = 2.0f;
        doorInteract->onInteract = [this, doorInteract, doorObj]() {
            if (doorOpen) {
                std::cout << "[INTERACTION] Door is already open.\n";
                return;
            }
            if (!hasKey) {
                std::cout << "[INTERACTION] The door is locked. You need a key.\n";
                doorInteract->actionText = "Locked (need key)";
            } else {
                doorOpen = true;
                doorObj->active = false; // Remove door
                std::cout << "[INTERACTION] You unlocked the door!\n";
            }
        };
        doorInteract->actionText = "Open Door";

        // 3. Treasure chest
        auto chestObj = CreateGameObject("Chest");
        chestObj->transform->position = Vector3(7.5f, 7.5f, 0.2f);
        chestObj->transform->localScale = Vector2(0.5f, 0.5f);

        auto chestSR = chestObj->AddComponent<SpriteRenderer>();
        chestSR->SetSprite(chestSprite);
        chestSR->billboard = true;
        chestSR->sortingOrder = 100;
        chestSR->highlightColor = Color(0.3f, 0.2f, 0.0f, 0.0f); // Gold glow

        auto chestInteract = chestObj->AddComponent<Interactable>();
        chestInteract->actionText = "Open Chest";
        chestInteract->maxDistance = 2.5f;
        chestInteract->onInteract = [this, chestInteract]() {
            if (!chestLooted) {
                chestLooted = true;
                chestInteract->actionText = "Empty";
                std::cout << "[INTERACTION] You found treasure inside the chest!\n";
                std::cout << "=== DEMO COMPLETE! ===\n";
            } else {
                std::cout << "[INTERACTION] The chest is empty.\n";
            }
        };

        std::cout << "Scene loaded. Look at objects and press E to interact!\n";
        std::cout << "Objective: Find the key, open the door, loot the chest.\n\n";
    }

    void OnUpdate() override {
        // Animate interactable objects (floating effect)
        static float animTime = 0.0f;
        animTime += Time::deltaTime;

        auto keyObj = Find("Key");
        if (keyObj && keyObj->active) {
            keyObj->transform->position.z = 0.25f + std::sin(animTime * 2.0f) * 0.1f;
        }

        auto chestObj = Find("Chest");
        if (chestObj && chestObj->active) {
            chestObj->transform->position.z = 0.2f + std::sin(animTime * 1.5f + 1.0f) * 0.05f;
        }
    }
};

} // namespace Game
