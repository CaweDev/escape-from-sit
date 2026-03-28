#include "LaptopItem.hpp"
#include "Game/SecurityGuard/SecurityGuard.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/Audio/SoundManager.hpp"

namespace Game
{

    void LaptopItem::Use(Engine::GameObject *user)
    {
        gameObject->active = true;
        gameObject->transform->position = user->transform->position;

        auto rb = gameObject->GetComponent<Engine::Rigidbody2D>();
        if (!rb)
        {
            rb = gameObject->AddComponent<Engine::Rigidbody2D>();
        }
        rb->bodyType = Engine::RigidbodyType2D::Dynamic;
        rb->drag = 2.0f;

        Engine::Vector2 throwDir(1.0f, 0.0f);
        rb->velocity = Engine::Vector2(0, 0);
        rb->AddImpulse(throwDir * 15.0f);

        isThrown = true;
    }

    void LaptopItem::Update()
    {
        if (!isThrown)
            return;

        fuseTimer -= Engine::Time::deltaTime;
        if (fuseTimer <= 0.0f)
        {
            Engine::SoundManager::PlaySFX("loud_noise");

            auto activeScene = Engine::SceneManager::GetActiveScene();
            if (activeScene)
            {
                for (auto &obj : activeScene->GetGameObjects())
                {
                    auto guard = obj->GetComponent<SecurityGuard>();
                    if (guard)
                    {
                        guard->OnCCTVSpotted(gameObject->transform->position);
                    }
                }
            }
            Engine::GameObject::Destroy(gameObject);
        }
    }

}