#include "LaptopItem.hpp"
#include "Game/SecurityGuard/SecurityGuard.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Scene/SceneManager.hpp"
#include "Engine/Audio/SoundManager.hpp"
#include "Game/NPC/NPC.hpp"

namespace Game
{
    void LaptopItem::Initialize()
    {
        Item::Initialize();
        rigidbody2D = gameObject->GetComponent<Engine::Rigidbody2D>();
    }

    void LaptopItem::Use(Engine::GameObject *user)
    {
        gameObject->active = true;
        this->interactable->enabled = false;
        transform->position = user->transform->position;
        gameObject->transform->position.z = 0.5f;
        rigidbody2D->bodyType = Engine::RigidbodyType2D::Dynamic;
        rigidbody2D->drag = 3.0f;
        rigidbody2D->gravityScale = 1.0f;

        rigidbody2D->currentHeight = 0.5f;
        rigidbody2D->verticalVelocity = 2.0f;
        rigidbody2D->isGrounded = false;
        rigidbody2D->velocity = Engine::Vector2(0, 0);

        float rot = AEDegToRad(user->transform->rotation);
        Engine::Vector2 throwDir(cosf(rot), sinf(rot));
        rigidbody2D->AddImpulse(throwDir * 10.0f);

        isThrown = true;
        hasLanded = false;
        timer = 1.5f;
    }

    void LaptopItem::Update()
    {
        if (!isThrown)
            return;

        if (rigidbody2D && rigidbody2D->isGrounded && !hasLanded)
        {
            hasLanded = true;
        }

        if (hasLanded)
        {
            timer -= Engine::Time::deltaTime;

            if (timer >= 0.0f)
            {
                Engine::SoundManager::PlaySFX("loud_noise");

                Engine::Vector2 noisePos = gameObject->transform->position;
                auto activeScene = Engine::SceneManager::GetActiveScene();

                const float distractionRadius = 15.0f;
                const float radiusSq = distractionRadius * distractionRadius;

                if (activeScene)
                {
                    for (auto &obj : activeScene->GetGameObjects())
                    {
                        if (!obj || !obj->active || !obj->transform)
                            continue;

                        float dx = obj->transform->position.x - noisePos.x;
                        float dy = obj->transform->position.y - noisePos.y;
                        if ((dx * dx + dy * dy) > radiusSq)
                            continue;

                        auto guard = obj->GetComponent<SecurityGuard>();
                        if (guard)
                        {
                            guard->OnDistracted(noisePos);
                            distractedEntities.push_back(obj.get());
                            continue;
                        }

                        auto npc = obj->GetComponent<NPC>();
                        if (npc)
                        {
                            npc->OnDistracted(noisePos);
                            distractedEntities.push_back(obj.get());
                        }
                    }
                }
            }
            else
            {
                Engine::GameObject::Destroy(gameObject);
            }
        }
    }

    void LaptopItem::OnDestroy()
    {
        for (auto *obj : distractedEntities)
        {
            if (!obj || obj->isPendingDestruction)
                continue;

            auto guard = obj->GetComponent<SecurityGuard>();
            if (guard)
            {
                guard->ClearDistraction();
                continue;
            }

            auto npc = obj->GetComponent<NPC>();
            if (npc)
            {
                npc->ClearDistraction();
            }
        }
        distractedEntities.clear();
    }
}
