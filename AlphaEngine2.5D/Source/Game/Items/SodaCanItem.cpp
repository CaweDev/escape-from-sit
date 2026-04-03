/*!
 * \file AlphaEngine2.5D\Source\Game\Items\SodaCanItem.cpp
 * \author DigiPen Student
 * \date 2026-03-29
 * \brief [AUTO] Added Doxygen file header
 */

#include "SodaCanItem.hpp"
#include "Engine/Physics/Physics2D.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Physics/Collider2D.hpp"
#include "Engine/Audio/SoundManager.hpp"
#include "Engine/Core/GameObject.hpp"

#include "Game/Player/Player.hpp"
namespace Game
{


    void SodaCanItem::Use(Engine::GameObject *user)
    {
        // Engine::SoundManager::PlaySFX("soda_spray");
       if (auto player = user->GetComponent<Game::Player>())
       {
        player->ResetStamina();
       }

        Engine::GameObject::Destroy(gameObject);
    }

}