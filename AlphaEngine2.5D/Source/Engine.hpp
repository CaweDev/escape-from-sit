#ifndef _ENGINE_HPP
#define _ENGINE_HPP
/**
 * @file Engine.hpp
 * @brief Master header for Alpha Engine 2.5D
 */

// Audio
#include "Engine/Audio/AudioClip.hpp"
#include "Engine/Audio/AudioSource.hpp"
#include "Engine/Audio/SoundManager.hpp"

// Core
#include "Engine/Core/Application.hpp"
#include "Engine/Core/Component.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/Transform.hpp"
#include "Engine/Core/AssetManager.hpp"

// Graphics
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Graphics/Color.hpp"
#include "Engine/Graphics/IRenderer.hpp"
#include "Engine/Graphics/Sprite.hpp"
#include "Engine/Graphics/SpriteRenderer.hpp"
#include "Engine/Graphics/SpriteSheet.hpp"
#include "Engine/Graphics/SpriteSheetRenderer.hpp"
#include "Engine/Graphics/TileMapRenderer.hpp"
#include "Engine/Graphics/ZBuffer.hpp"

// Input
#include "Engine/Input/Input.hpp"
#include "Engine/Input/KeyCode.hpp"

// Math
#include "Engine/Math/Mathf.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"

// Physics
#include "Engine/Physics/BoxCollider2D.hpp"
#include "Engine/Physics/CircleCollider2D.hpp"
#include "Engine/Physics/Collider2D.hpp"
#include "Engine/Physics/Collision2D.hpp"
#include "Engine/Physics/Physics2D.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Physics/TileMapCollider2D.hpp"

// Raycasting
#include "Engine/Raycasting/ProjectionHelper.hpp"

// Interaction
#include "Engine/Interaction/Interactable.hpp"
#include "Engine/Interaction/InteractionController.hpp"

// Scene
#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/SceneManager.hpp"

// TileMap
#include "Engine/TileMap/TileMap.hpp"

// UI
#include "Engine/UI/UI.hpp"
#endif