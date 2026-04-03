/* Start Header ************************************************************************/
/*!
\file ReflectionRegistry.cpp
\author
\par
\date	March, 29, 2026
\brief		[AUTO] ReflectionRegistry

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "ReflectionRegistry.hpp"

#include "Reflection.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Graphics/SpriteSheetRenderer.hpp"
#include "Engine/Interaction/Interactable.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Physics/CircleCollider2D.hpp"
#include "Engine/Physics/BoxCollider2D.hpp"
#include "Engine/Graphics/DoorRenderer.hpp"
#include "Engine/Tilemap/Door.hpp"

#include "game/AccessCard/AccessCard.hpp"
#include "game/CCTV/CCTVCamera.hpp"
#include "game/ControlRoomDoor/ControlRoomDoor.hpp"
#include "game/NPC/NPC.hpp"
#include "game/Player/Player.hpp"
#include "game/PowerUp/PowerUp.hpp"
#include "game/Puzzle/PuzzleObject.hpp"

#include "game/FPSController.hpp"
#include "game/Scenes/FPSScene/UI/PlayerHUD.hpp"
#include "game/Scenes/FPSScene/UI/PauseMenu.hpp"
#include "game/Scenes/FPSScene/UI/LoseScreen.hpp"
#include "game/Scenes/FPSScene/UI/WinScreen.hpp"
#include "game/Scenes/FPSScene/UI/Hands67.hpp"
#include "game/Scenes/MainMenuScene/UI/ControlsMenu.hpp"
#include "game/Player/PlayerPowerUpController.hpp"
#include "game/SecurityGuard/SecurityGuard.hpp"
#include "game/CCTV/CCTVTerminal.hpp"

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

    void RegisterSpriteSheetRenderer()
    {
        Registry::RegisterClassType<Engine::SpriteSheetRenderer>("SpriteSheetRenderer");

        Registry::RegisterFieldType<Engine::SpriteSheetRenderer>("spriteSheetPath", offsetof(Engine::SpriteSheetRenderer, spriteSheetPath), DataType::SpriteSheet);
        Registry::RegisterFieldType<Engine::SpriteSheetRenderer>("color", offsetof(Engine::SpriteSheetRenderer, color), DataType::Color);
        Registry::RegisterFieldType<Engine::SpriteSheetRenderer>("sortingOrder", offsetof(Engine::SpriteSheetRenderer, sortingOrder), DataType::Int);
        Registry::RegisterFieldType<Engine::SpriteSheetRenderer>("flipX", offsetof(Engine::SpriteSheetRenderer, flipX), DataType::Bool);
        Registry::RegisterFieldType<Engine::SpriteSheetRenderer>("flipY", offsetof(Engine::SpriteSheetRenderer, flipY), DataType::Bool);
        Registry::RegisterFieldType<Engine::SpriteSheetRenderer>("billboard", offsetof(Engine::SpriteSheetRenderer, billboard), DataType::Bool);
    }

    void RegisterDoorRenderer()
    {
        Registry::RegisterClassType<Engine::DoorRenderer>("DoorRenderer");

        Registry::RegisterFieldType<Engine::DoorRenderer>("spritePath", offsetof(Engine::DoorRenderer, spritePath), DataType::Sprite);
        Registry::RegisterFieldType<Engine::DoorRenderer>("colorNS", offsetof(Engine::DoorRenderer, colorNS), DataType::Color);
        Registry::RegisterFieldType<Engine::DoorRenderer>("colorEW", offsetof(Engine::DoorRenderer, colorEW), DataType::Color);
        Registry::RegisterFieldType<Engine::DoorRenderer>("fov", offsetof(Engine::DoorRenderer, fov), DataType::Float);
    }

    void RegisterDoor()
    {
        Registry::RegisterClassType<Engine::Door>("Door");

        Registry::RegisterFieldType<Engine::Door>("tileX", offsetof(Engine::Door, tileX), DataType::Int);
        Registry::RegisterFieldType<Engine::Door>("tileY", offsetof(Engine::Door, tileY), DataType::Int);
        Registry::RegisterFieldType<Engine::Door>("doorType", offsetof(Engine::Door, doorType), DataType::Int);

        Registry::RegisterFieldType<Engine::Door>("facing", offsetof(Engine::Door, facing), DataType::Int);
        Registry::RegisterFieldType<Engine::Door>("state", offsetof(Engine::Door, state), DataType::Int);

        Registry::RegisterFieldType<Engine::Door>("openAmount", offsetof(Engine::Door, openAmount), DataType::Float);
        Registry::RegisterFieldType<Engine::Door>("animationSpeed", offsetof(Engine::Door, animationSpeed), DataType::Float);
        Registry::RegisterFieldType<Engine::Door>("autoCloseDelay", offsetof(Engine::Door, autoCloseDelay), DataType::Float);
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

    void RegisterAccessCard()
    {
        Registry::RegisterClassType<Game::AccessCard>("AccessCard");
        Registry::RegisterFieldType<Game::AccessCard>("CardColor", offsetof(Game::AccessCard, cardColor), DataType::Color);
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
        Registry::RegisterFieldType<Game::NPC>("isDistracted", offsetof(Game::NPC, isDistracted), DataType::Bool);
        Registry::RegisterFieldType<Game::NPC>("distractionPos", offsetof(Game::NPC, distractionPos), DataType::Vector2);
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
        Registry::RegisterClassType<Game::PuzzleObject>("PuzzleObject");

        Registry::RegisterFieldType<Game::PuzzleObject>("puzzleCompleted", offsetof(Game::PuzzleObject, puzzleCompleted), DataType::Bool);
        Registry::RegisterFieldType<Game::PuzzleObject>("isReading", offsetof(Game::PuzzleObject, isReading), DataType::Bool);
    }

    void RegisterRigidbody2D()
    {
        Registry::RegisterClassType<Engine::Rigidbody2D>("Rigidbody2D");

        Registry::RegisterFieldType<Engine::Rigidbody2D>("velocity", offsetof(Engine::Rigidbody2D, velocity), DataType::Vector2);
        Registry::RegisterFieldType<Engine::Rigidbody2D>("verticalVelocity", offsetof(Engine::Rigidbody2D, verticalVelocity), DataType::Float);
        Registry::RegisterFieldType<Engine::Rigidbody2D>("mass", offsetof(Engine::Rigidbody2D, mass), DataType::Float);
        Registry::RegisterFieldType<Engine::Rigidbody2D>("drag", offsetof(Engine::Rigidbody2D, drag), DataType::Float);
        Registry::RegisterFieldType<Engine::Rigidbody2D>("gravityScale", offsetof(Engine::Rigidbody2D, gravityScale), DataType::Float);
        Registry::RegisterFieldType<Engine::Rigidbody2D>("bodyType", offsetof(Engine::Rigidbody2D, bodyType), DataType::Int);
        Registry::RegisterFieldType<Engine::Rigidbody2D>("freezeRotation", offsetof(Engine::Rigidbody2D, freezeRotation), DataType::Bool);

        Registry::RegisterFieldType<Engine::Rigidbody2D>("currentHeight", offsetof(Engine::Rigidbody2D, currentHeight), DataType::Float);
        Registry::RegisterFieldType<Engine::Rigidbody2D>("standingHeight", offsetof(Engine::Rigidbody2D, standingHeight), DataType::Float);
        Registry::RegisterFieldType<Engine::Rigidbody2D>("crouchHeight", offsetof(Engine::Rigidbody2D, crouchHeight), DataType::Float);
        Registry::RegisterFieldType<Engine::Rigidbody2D>("collisionHeight", offsetof(Engine::Rigidbody2D, collisionHeight), DataType::Float);
        Registry::RegisterFieldType<Engine::Rigidbody2D>("isCrouching", offsetof(Engine::Rigidbody2D, isCrouching), DataType::Bool);
        Registry::RegisterFieldType<Engine::Rigidbody2D>("isGrounded", offsetof(Engine::Rigidbody2D, isGrounded), DataType::Bool);
    }

    void RegisterCamera()
    {
        Registry::RegisterClassType<Engine::Camera>("Camera");
        Registry::RegisterFieldType<Engine::Camera>("backgroundColor", offsetof(Engine::Camera, backgroundColor), DataType::Color);
        Registry::RegisterFieldType<Engine::Camera>("fogEnabled", offsetof(Engine::Camera, fogEnabled), DataType::Bool);
        Registry::RegisterFieldType<Engine::Camera>("fogDistance", offsetof(Engine::Camera, fogDistance), DataType::Float);
        Registry::RegisterFieldType<Engine::Camera>("fogMinVisibility", offsetof(Engine::Camera, fogMinVisibility), DataType::Float);
        Registry::RegisterFieldType<Engine::Camera>("fogMaxVisibility", offsetof(Engine::Camera, fogMaxVisibility), DataType::Float);
    }

    void RegisterBoxCollider2D()
    {
        Registry::RegisterClassType<Engine::BoxCollider2D>("BoxCollider2D");
        Registry::RegisterFieldType<Engine::BoxCollider2D>("size", offsetof(Engine::BoxCollider2D, size), DataType::Vector2);
        Registry::RegisterFieldType<Engine::BoxCollider2D>("minHeight", offsetof(Engine::BoxCollider2D, minHeight), DataType::Float);
        Registry::RegisterFieldType<Engine::BoxCollider2D>("maxHeight", offsetof(Engine::BoxCollider2D, maxHeight), DataType::Float);
    }

    void RegisterCircleCollider2D()
    {
        Registry::RegisterClassType<Engine::CircleCollider2D>("CircleCollider2D");
        Registry::RegisterFieldType<Engine::CircleCollider2D>("radius", offsetof(Engine::CircleCollider2D, radius), DataType::Float);
        Registry::RegisterFieldType<Engine::CircleCollider2D>("minHeight", offsetof(Engine::CircleCollider2D, minHeight), DataType::Float);
        Registry::RegisterFieldType<Engine::CircleCollider2D>("maxHeight", offsetof(Engine::CircleCollider2D, maxHeight), DataType::Float);
    }

    void RegisterFPSController()
    {
        Registry::RegisterClassType<Game::FPSController>("FPSController");
        Registry::RegisterFieldType<Game::FPSController>("moveSpeed", offsetof(Game::FPSController, moveSpeed), DataType::Float);
        Registry::RegisterFieldType<Game::FPSController>("sprintSpeed", offsetof(Game::FPSController, sprintSpeed), DataType::Float);
        Registry::RegisterFieldType<Game::FPSController>("rotateSpeed", offsetof(Game::FPSController, rotateSpeed), DataType::Float);
        Registry::RegisterFieldType<Game::FPSController>("mouseSensitivity", offsetof(Game::FPSController, mouseSensitivity), DataType::Float);
        Registry::RegisterFieldType<Game::FPSController>("jumpForce", offsetof(Game::FPSController, jumpForce), DataType::Float);
        Registry::RegisterFieldType<Game::FPSController>("eyeHeight", offsetof(Game::FPSController, eyeHeight), DataType::Float);
        Registry::RegisterFieldType<Game::FPSController>("viewHeightSmoothSpeed", offsetof(Game::FPSController, viewHeightSmoothSpeed), DataType::Float);
    }

    void RegisterInteractionController()
    {
        Registry::RegisterClassType<Engine::InteractionController>("InteractionController");
        Registry::RegisterFieldType<Engine::InteractionController>("interactKey", offsetof(Engine::InteractionController, interactKey), DataType::KeyCode);
    }

    void RegisterPlayerUIComponents()
    {
        Registry::RegisterClassType<Game::PlayerHUD>("PlayerHUD");
        Registry::RegisterClassType<Game::PauseMenu>("PauseMenu");
        Registry::RegisterClassType<Game::Hands67>("Hands67");
        Registry::RegisterClassType<Game::LoseScreen>("LoseScreen");
        Registry::RegisterClassType<Game::WinScreen>("WinScreen");
        Registry::RegisterClassType<Game::ControlsMenu>("ControlsMenu");
        Registry::RegisterClassType<Game::PlayerPowerUpController>("PlayerPowerUpController");
    }

    void RegisterSecurityGuard()
    {
        Registry::RegisterClassType<Game::SecurityGuard>("SecurityGuard");

        Registry::RegisterFieldType<Game::SecurityGuard>("homeZoneId", offsetof(Game::SecurityGuard, homeZoneId), DataType::Int);
        Registry::RegisterFieldType<Game::SecurityGuard>("patrolSpeed", offsetof(Game::SecurityGuard, patrolSpeed), DataType::Float);
        Registry::RegisterFieldType<Game::SecurityGuard>("chaseSpeed", offsetof(Game::SecurityGuard, chaseSpeed), DataType::Float);
        Registry::RegisterFieldType<Game::SecurityGuard>("searchSpeed", offsetof(Game::SecurityGuard, searchSpeed), DataType::Float);
        Registry::RegisterFieldType<Game::SecurityGuard>("arriveDistance", offsetof(Game::SecurityGuard, arriveDistance), DataType::Float);
        Registry::RegisterFieldType<Game::SecurityGuard>("detectRadius", offsetof(Game::SecurityGuard, detectRadius), DataType::Float);
        Registry::RegisterFieldType<Game::SecurityGuard>("catchRadius", offsetof(Game::SecurityGuard, catchRadius), DataType::Float);
        Registry::RegisterFieldType<Game::SecurityGuard>("maxLostSightTime", offsetof(Game::SecurityGuard, maxLostSightTime), DataType::Float);
        Registry::RegisterFieldType<Game::SecurityGuard>("searchDuration", offsetof(Game::SecurityGuard, searchDuration), DataType::Float);
    }

    void RegisterCCTVTerminal()
    {
        Registry::RegisterClassType<Game::CCTVTerminal>("CCTVTerminal");

        // Configuration
        Registry::RegisterFieldType<Game::CCTVTerminal>("controlledZone", offsetof(Game::CCTVTerminal, controlledZone), DataType::Int);

        // State
        Registry::RegisterFieldType<Game::CCTVTerminal>("isDisabled", offsetof(Game::CCTVTerminal, isDisabled), DataType::Bool);
        Registry::RegisterFieldType<Game::CCTVTerminal>("disabledTimer", offsetof(Game::CCTVTerminal, disabledTimer), DataType::Float);
        Registry::RegisterFieldType<Game::CCTVTerminal>("disabledDuration", offsetof(Game::CCTVTerminal, disabledDuration), DataType::Float);

        // Repair system
        Registry::RegisterFieldType<Game::CCTVTerminal>("guardIsRepairing", offsetof(Game::CCTVTerminal, guardIsRepairing), DataType::Bool);
        Registry::RegisterFieldType<Game::CCTVTerminal>("repairProgress", offsetof(Game::CCTVTerminal, repairProgress), DataType::Float);
        Registry::RegisterFieldType<Game::CCTVTerminal>("repairTime", offsetof(Game::CCTVTerminal, repairTime), DataType::Float);
        Registry::RegisterFieldType<Game::CCTVTerminal>("repairMaxDistance", offsetof(Game::CCTVTerminal, repairMaxDistance), DataType::Float);

        // Interaction
        Registry::RegisterFieldType<Game::CCTVTerminal>("interactionTime", offsetof(Game::CCTVTerminal, interactionTime), DataType::Float);
        Registry::RegisterFieldType<Game::CCTVTerminal>("interactionProgress", offsetof(Game::CCTVTerminal, interactionProgress), DataType::Float);
        Registry::RegisterFieldType<Game::CCTVTerminal>("playerIsInteracting", offsetof(Game::CCTVTerminal, playerIsInteracting), DataType::Bool);
        Registry::RegisterFieldType<Game::CCTVTerminal>("cooldownTimer", offsetof(Game::CCTVTerminal, cooldownTimer), DataType::Float);
        Registry::RegisterFieldType<Game::CCTVTerminal>("cooldownDuration", offsetof(Game::CCTVTerminal, cooldownDuration), DataType::Float);
        Registry::RegisterFieldType<Game::CCTVTerminal>("interactMaxDistance", offsetof(Game::CCTVTerminal, interactMaxDistance), DataType::Float);

        // Floating icon & Visuals
        Registry::RegisterFieldType<Game::CCTVTerminal>("floatAnimTime", offsetof(Game::CCTVTerminal, floatAnimTime), DataType::Float);
        Registry::RegisterFieldType<Game::CCTVTerminal>("floatSpeed", offsetof(Game::CCTVTerminal, floatSpeed), DataType::Float);
        Registry::RegisterFieldType<Game::CCTVTerminal>("floatAmplitude", offsetof(Game::CCTVTerminal, floatAmplitude), DataType::Float);
        Registry::RegisterFieldType<Game::CCTVTerminal>("floatBaseHeightOffset", offsetof(Game::CCTVTerminal, floatBaseHeightOffset), DataType::Float);
        Registry::RegisterFieldType<Game::CCTVTerminal>("pulseSpeed", offsetof(Game::CCTVTerminal, pulseSpeed), DataType::Float);

        // Text configuration
        Registry::RegisterFieldType<Game::CCTVTerminal>("interactText", offsetof(Game::CCTVTerminal, interactText), DataType::String);
        Registry::RegisterFieldType<Game::CCTVTerminal>("zoneHackTextPrefix", offsetof(Game::CCTVTerminal, zoneHackTextPrefix), DataType::String);
        Registry::RegisterFieldType<Game::CCTVTerminal>("zoneHackTextSuffix", offsetof(Game::CCTVTerminal, zoneHackTextSuffix), DataType::String);
        Registry::RegisterFieldType<Game::CCTVTerminal>("disabledTextSuffix", offsetof(Game::CCTVTerminal, disabledTextSuffix), DataType::String);
        Registry::RegisterFieldType<Game::CCTVTerminal>("cooldownText", offsetof(Game::CCTVTerminal, cooldownText), DataType::String);
        Registry::RegisterFieldType<Game::CCTVTerminal>("repairingText", offsetof(Game::CCTVTerminal, repairingText), DataType::String);
        Registry::RegisterFieldType<Game::CCTVTerminal>("hackStatusPrefix", offsetof(Game::CCTVTerminal, hackStatusPrefix), DataType::String);
        Registry::RegisterFieldType<Game::CCTVTerminal>("hackStatusDuration", offsetof(Game::CCTVTerminal, hackStatusDuration), DataType::Float);

        // Colors
        Registry::RegisterFieldType<Game::CCTVTerminal>("activeColor", offsetof(Game::CCTVTerminal, activeColor), DataType::Color);
        Registry::RegisterFieldType<Game::CCTVTerminal>("hackingColor", offsetof(Game::CCTVTerminal, hackingColor), DataType::Color);
        Registry::RegisterFieldType<Game::CCTVTerminal>("disabledColor", offsetof(Game::CCTVTerminal, disabledColor), DataType::Color);
        Registry::RegisterFieldType<Game::CCTVTerminal>("repairingColor", offsetof(Game::CCTVTerminal, repairingColor), DataType::Color);
    }

    void
    ReflectionRegistry::InitializeMetaRegistry()
    {
        Registry::types.clear();
        Registry::nameToType.clear();

        RegisterTransform();
        RegisterSpriteRenderer();
        RegisterSpriteSheetRenderer();
        RegisterRigidbody2D();

        RegisterInteractable();
        RegisterAccessCard();
        RegisterCCTVCamera();
        RegisterControlRoomDoor();
        RegisterDoor();
        RegisterNPC();
        RegisterPlayer();
        RegisterPowerUpObject();
        RegisterPuzzleObject();

        RegisterCamera();
        RegisterCircleCollider2D();
        RegisterFPSController();
        RegisterInteractionController();
        RegisterPlayerUIComponents();
        RegisterSecurityGuard();
        RegisterCCTVTerminal();
        RegisterDoorRenderer();
        RegisterBoxCollider2D();
    }

}
