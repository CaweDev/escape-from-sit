#include "ReflectionRegistry.hpp"

#include "Reflection.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Interaction/Interactable.hpp"

#include "game/Items/Item.hpp"
#include "game/AccessCard/AccessCard.hpp"
#include "game/CCTV/CCTVCamera.hpp"
#include "game/ControlRoomDoor/ControlRoomDoor.hpp"
#include "game/NPC/NPC.hpp"
#include "game/Player/Player.hpp"
#include "game/PowerUp/PowerUp.h"
#include "game/Puzzle/PuzzleObject.h"

namespace AEEditor
{

    void RegisterTransform()
    {

        Registry::RegisterClassType<Engine::Transform>("Transform");
        Registry::RegisterFieldType<Engine::Transform>("Position", offsetof(Engine::Transform, position), DataType::Vector3);
        Registry::RegisterFieldType<Engine::Transform>("Rotation", offsetof(Engine::Transform, rotation), DataType::Float);
        Registry::RegisterFieldType<Engine::Transform>("Scale", offsetof(Engine::Transform, localScale), DataType::Vector2);
    }

    void RegisterSpriteRenderer()
    {
        AEEditor::Registry::RegisterClassType<Engine::SpriteRenderer>("SpriteRenderer");
        AEEditor::Registry::RegisterFieldType<Engine::SpriteRenderer>("spritePath", offsetof(Engine::SpriteRenderer, spritePath), AEEditor::DataType::Sprite);
        AEEditor::Registry::RegisterFieldType<Engine::SpriteRenderer>("sortingOrder", offsetof(Engine::SpriteRenderer, sortingOrder), AEEditor::DataType::Int);
        AEEditor::Registry::RegisterFieldType<Engine::SpriteRenderer>("flipX", offsetof(Engine::SpriteRenderer, flipX), AEEditor::DataType::Bool);
        AEEditor::Registry::RegisterFieldType<Engine::SpriteRenderer>("flipY", offsetof(Engine::SpriteRenderer, flipY), AEEditor::DataType::Bool);
        AEEditor::Registry::RegisterFieldType<Engine::SpriteRenderer>("billboard", offsetof(Engine::SpriteRenderer, billboard), AEEditor::DataType::Bool);
    }

    void RegisterInteractable()
    {
        AEEditor::Registry::RegisterClassType<Engine::Interactable>("Interactable");
        AEEditor::Registry::RegisterFieldType<Engine::Interactable>("actionText", offsetof(Engine::Interactable, actionText), AEEditor::DataType::String);
        AEEditor::Registry::RegisterFieldType<Engine::Interactable>("maxDistance", offsetof(Engine::Interactable, maxDistance), AEEditor::DataType::Float);
        AEEditor::Registry::RegisterFieldType<Engine::Interactable>("hitShape", offsetof(Engine::Interactable, hitShape), AEEditor::DataType::Int);
        AEEditor::Registry::RegisterFieldType<Engine::Interactable>("requireHold", offsetof(Engine::Interactable, requireHold), AEEditor::DataType::Bool);
        AEEditor::Registry::RegisterFieldType<Engine::Interactable>("holdDuration", offsetof(Engine::Interactable, holdDuration), AEEditor::DataType::Float);
    }

    void RegisterItem()
    {
        Registry::RegisterClassType<Game::Item>("Item");
        Registry::RegisterFieldType<Game::Item>("Position", offsetof(Game::Item, isCollected), DataType::Bool);
        Registry::RegisterFieldType<Game::Item>("Rotation", offsetof(Game::Item, floatTime), DataType::Float);
        Registry::RegisterFieldType<Game::Item>("Scale", offsetof(Game::Item, baseZ), DataType::Float);
    }

    void RegisterAccessCard()
    {
        Registry::RegisterClassType<Game::AccessCard>("AccessCard");
        // Registry::RegisterFieldType<Game::AccessCard>("CardColor", offsetof(Game::AccessCard, cardColor), DataType::Int);
        Registry::RegisterFieldType<Game::AccessCard>("CardId", offsetof(Game::AccessCard, cardId), DataType::Int);
        Registry::RegisterFieldType<Game::AccessCard>("FloatTime", offsetof(Game::AccessCard, floatTime), DataType::Float);
        Registry::RegisterFieldType<Game::AccessCard>("BaseZ", offsetof(Game::AccessCard, baseZ), DataType::Float);
        Registry::RegisterFieldType<Game::AccessCard>("FloatHeight", offsetof(Game::AccessCard, floatHeight), DataType::Float);
        Registry::RegisterFieldType<Game::AccessCard>("FloatSpeed", offsetof(Game::AccessCard, floatSpeed), DataType::Float);
        Registry::RegisterFieldType<Game::AccessCard>("SpawnSafeDelay", offsetof(Game::AccessCard, spawnSafeDelay), DataType::Float);
        Registry::RegisterFieldType<Game::AccessCard>("SpawnSafeTimer", offsetof(Game::AccessCard, spawnSafeTimer), DataType::Float);
    }

    void RegisterCCTVCamera()
    {
        Registry::RegisterClassType<Game::CCTVCamera>("CCTVCamera");

        Registry::RegisterFieldType<Game::CCTVCamera>("hasLastSeen", offsetof(Game::CCTVCamera, hasLastSeen), DataType::Bool);
        Registry::RegisterFieldType<Game::CCTVCamera>("lastSeenTile", offsetof(Game::CCTVCamera, lastSeenTile), DataType::Vector2);
        Registry::RegisterFieldType<Game::CCTVCamera>("lastSeenWorld", offsetof(Game::CCTVCamera, lastSeenWorld), DataType::Vector2);
        Registry::RegisterFieldType<Game::CCTVCamera>("id", offsetof(Game::CCTVCamera, id), DataType::Int);

        Registry::RegisterFieldType<Game::CCTVCamera>("detectRadius", offsetof(Game::CCTVCamera, detectRadius), DataType::Float);
        Registry::RegisterFieldType<Game::CCTVCamera>("coneWidth", offsetof(Game::CCTVCamera, coneWidth), DataType::Float);
        Registry::RegisterFieldType<Game::CCTVCamera>("maxRange", offsetof(Game::CCTVCamera, maxRange), DataType::Float);
        Registry::RegisterFieldType<Game::CCTVCamera>("needLOS", offsetof(Game::CCTVCamera, needLOS), DataType::Bool);

        Registry::RegisterFieldType<Game::CCTVCamera>("showVision", offsetof(Game::CCTVCamera, showVision), DataType::Bool);
        Registry::RegisterFieldType<Game::CCTVCamera>("rayCount", offsetof(Game::CCTVCamera, rayCount), DataType::Int);
        Registry::RegisterFieldType<Game::CCTVCamera>("rayStepPerUnit", offsetof(Game::CCTVCamera, rayStepPerUnit), DataType::Float);

        Registry::RegisterFieldType<Game::CCTVCamera>("isSeeingPlayer", offsetof(Game::CCTVCamera, isSeeingPlayer), DataType::Bool);
        Registry::RegisterFieldType<Game::CCTVCamera>("justSpotted", offsetof(Game::CCTVCamera, justSpotted), DataType::Bool);

        Registry::RegisterFieldType<Game::CCTVCamera>("isDisabled", offsetof(Game::CCTVCamera, isDisabled), DataType::Bool);
        Registry::RegisterFieldType<Game::CCTVCamera>("disabledTimeRemaining", offsetof(Game::CCTVCamera, disabledTimeRemaining), DataType::Float);
    }

    void RegisterControlRoomDoor()
    {
        Registry::RegisterClassType<Game::ControlRoomDoor>("ControlRoomDoor");

        Registry::RegisterFieldType<Game::ControlRoomDoor>("zoneId", offsetof(Game::ControlRoomDoor, zoneId), DataType::Int);

        Registry::RegisterFieldType<Game::ControlRoomDoor>("holdProgress", offsetof(Game::ControlRoomDoor, holdProgress), DataType::Float);
        Registry::RegisterFieldType<Game::ControlRoomDoor>("holdTimeRequired", offsetof(Game::ControlRoomDoor, holdTimeRequired), DataType::Float);
        Registry::RegisterFieldType<Game::ControlRoomDoor>("isHolding", offsetof(Game::ControlRoomDoor, isHolding), DataType::Bool);
    }

    void RegisterNPC()
    {
        Registry::RegisterClassType<Game::NPC>("NPC");

        Registry::RegisterFieldType<Game::NPC>("type", offsetof(Game::NPC, type), DataType::Int);
        Registry::RegisterFieldType<Game::NPC>("zoneId", offsetof(Game::NPC, zoneId), DataType::Int);

        Registry::RegisterFieldType<Game::NPC>("speed", offsetof(Game::NPC, speed), DataType::Float);
        Registry::RegisterFieldType<Game::NPC>("arriveDistance", offsetof(Game::NPC, arriveDistance), DataType::Float);

        Registry::RegisterFieldType<Game::NPC>("triggerRadius", offsetof(Game::NPC, triggerRadius), DataType::Float);
        Registry::RegisterFieldType<Game::NPC>("blockTime", offsetof(Game::NPC, blockTime), DataType::Float);

        Registry::RegisterFieldType<Game::NPC>("patrolRepathTime", offsetof(Game::NPC, patrolRepathTime), DataType::Float);
    }

    void RegisterPlayer()
    {
        Registry::RegisterClassType<Game::Player>("Player");

        Registry::RegisterFieldType<Game::Player>("isSprinting", offsetof(Game::Player, isSprinting), DataType::Bool);
    }

    void RegisterPowerUpObject()
    {
        Registry::RegisterClassType<Game::PowerUpObject>("PowerUpObject");

        Registry::RegisterFieldType<Game::PowerUpObject>("isCollected", offsetof(Game::PowerUpObject, isCollected), DataType::Bool);

        Registry::RegisterFieldType<Game::PowerUpObject>("floatTime", offsetof(Game::PowerUpObject, floatTime), DataType::Float);
        Registry::RegisterFieldType<Game::PowerUpObject>("baseZ", offsetof(Game::PowerUpObject, baseZ), DataType::Float);
        Registry::RegisterFieldType<Game::PowerUpObject>("floatHeight", offsetof(Game::PowerUpObject, floatHeight), DataType::Float);
        Registry::RegisterFieldType<Game::PowerUpObject>("floatSpeed", offsetof(Game::PowerUpObject, floatSpeed), DataType::Float);
    }

    void RegisterPuzzleObject()
    {
        Registry::RegisterClassType<PuzzleObject>("PuzzleObject");

        Registry::RegisterFieldType<PuzzleObject>("PuzzleCompleted", offsetof(PuzzleObject, PuzzleCompleted), DataType::Bool);
        Registry::RegisterFieldType<PuzzleObject>("isReading", offsetof(PuzzleObject, isReading), DataType::Bool);
    }

    void ReflectionRegistry::InitializeMetaRegistry()
    {
        Registry::types.clear();
        Registry::nameToType.clear();

        RegisterTransform();
        RegisterSpriteRenderer();
        RegisterInteractable();
        RegisterItem();
        RegisterAccessCard();
        RegisterCCTVCamera();
        RegisterControlRoomDoor();
        RegisterNPC();
        RegisterPlayer();
        RegisterPowerUpObject();
        RegisterPuzzleObject();
    }

}