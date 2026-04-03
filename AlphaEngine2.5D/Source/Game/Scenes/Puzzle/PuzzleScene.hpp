/**
 * @file PuzzleScene.hpp
 * @brief Puzzle scene definition
 */
#pragma once

#include "Engine/Scene/Scene.hpp"
#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Graphics/TileMapRenderer.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Physics/CircleCollider2D.hpp"
#include "Engine/Physics/TileMapCollider2D.hpp"
#include "Game/FPSController.hpp"
#include <Engine/Graphics/SpriteRenderer.hpp>
#include "Game/Player.hpp"
#include "Game/Puzzle/PuzzleManager.hpp"
#include "Engine/UI/Components/Text.hpp"
#include "Engine/UI/Core/UIManager.hpp"

namespace Game {

using namespace Engine;

/**
 * @class PuzzleScene
 * @brief Doom-style first-person demo with physics
 */
class PuzzleScene : public Scene {
public:
    PuzzleScene() : Scene("PuzzleScene") {}

    FPSController* playerController = nullptr;
    UI::Text* bookCountText = nullptr;
    UI::Canvas* hudCanvas = nullptr;
    int lastBookCount = -1;

    void OnLoad() override {
        // Define a simple maze level
        const int mapData[] = {
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
            1,0,1,1,1,0,1,1,1,1,0,1,1,1,0,1,
            1,0,1,0,0,0,0,0,0,0,0,0,0,1,0,1,
            1,0,1,0,1,1,1,0,1,1,1,0,0,1,0,1,
            1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1,
            1,0,1,0,1,0,1,1,1,0,1,0,1,1,0,1,
            1,0,1,0,0,0,1,0,1,0,0,0,0,1,0,1,
            1,0,1,1,1,0,1,0,1,0,1,1,0,1,0,1,
            1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
            1,0,1,1,1,1,1,0,1,1,1,1,1,1,0,1,
            1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        };
        
        // Create world object with TileMap
        auto world = CreateGameObject("World");
        auto map = world->AddComponent<TileMap>();
        map->LoadFromArray(mapData, 16, 13);

        auto renderer = world->AddComponent<TileMapRenderer>();
        renderer->wallColorNS = Color(0.8f, 0.2f, 0.2f, 1.0f);
        renderer->wallColorEW = Color(0.6f, 0.15f, 0.15f, 1.0f);
        renderer->ceilingColor = Color(0.1f, 0.1f, 0.2f, 1.0f);
        renderer->floorColor = Color(0.3f, 0.25f, 0.2f, 1.0f);
        {   // Load map sprites into TileMapRenderer
            auto skySprite = std::make_shared<Sprite>();
            skySprite->Load("Assets/Tilemap/modern_school_ceiling.png");
            auto groundSprite = std::make_shared<Sprite>();
            groundSprite->Load("Assets/Tilemap/modern_school_floor.png");
            auto wallSprite = std::make_shared<Sprite>();
            wallSprite->Load("Assets/Tilemap/modern_school_wall.png");
            // Set on TileMapRenderer
            renderer->SetCeilingSprite(skySprite);
            renderer->SetFloorSprite(groundSprite);
            renderer->SetWallSprite(wallSprite);
        }
        
        // Add tilemap collider for physics
        auto tileMapCollider = world->AddComponent<TileMapCollider2D>();
        tileMapCollider->minHeight = 0.0f;
        tileMapCollider->maxHeight = 3.0f; // Walls are 3 units tall
        
        // Create player with physics
        auto player = CreateGameObject("Player");
        player->AddComponent<Camera>();
        
        // Add physics components
        auto playerRb = player->AddComponent<Rigidbody2D>();
        playerRb->gravityScale = 1.0f;
        playerRb->drag = 5.0f; // Stop quickly when no input
        
        auto playerCollider = player->AddComponent<CircleCollider2D>();
        playerCollider->radius = 0.2f;
        playerCollider->minHeight = 0.0f;
        playerCollider->maxHeight = 1.8f;
        
        // Add controller (requires Rigidbody2D)
        playerController = player->AddComponent<FPSController>();
        player->AddComponent<Player>();
        player->transform->position = Vector2(1.5f, 1.5f);
        player->transform->rotation = 0.0f;
        

        // Book Count UI: create a Canvas and add the Text as a child so the UIManager renders it
        hudCanvas = UI::UIManager::GetInstance()->CreateCanvas();
        hudCanvas->SetReferenceResolution(1280.0f, 720.0f);
        hudCanvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);
        hudCanvas->SetSortOrder(100);

        bookCountText = hudCanvas->CreateChild<UI::Text>();
        bookCountText->name = "BookCountText";
        bookCountText->SetAnchors({ 0.0f, 1.0f }, { 0.0f, 1.0f }); // top-left anchor
        bookCountText->SetPivot({ 0.0f, 1.0f });                   // pivot at top-left
        bookCountText->SetAnchoredPosition({ 10.0f, -10.0f });     // 10px from top-left
        bookCountText->SetSizeDelta({ 0.0f, 0.0f });
        bookCountText->SetText("Books Collected: 0 / 6");
        bookCountText->SetFontSize(1.2f);
        bookCountText->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
        bookCountText->SetAnchor(UI::TextAnchor::UpperLeft);
        bookCountText->SetRenderOrder(1000);

        Game::PuzzleManager::BookDataInit();
        Game::PuzzleManager::Init();
        
    }
    
    void OnUpdate() override {
        // Physics and rendering handled by components
        if (Game::PuzzleManager::booksCollected != lastBookCount) {

            if (bookCountText != nullptr) {
                // Construct the string
                std::string uiString = "Books Collected: " + std::to_string(Game::PuzzleManager::booksCollected) + " / 6";

                // Pass it to your setter
                bookCountText->SetText(uiString);
            }

            // Save the current count so we don't run this code again until the next book
            lastBookCount = Game::PuzzleManager::booksCollected;
        }
    }
};

} // namespace Game
