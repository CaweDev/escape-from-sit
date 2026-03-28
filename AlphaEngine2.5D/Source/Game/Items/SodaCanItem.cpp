#include "SodaCanItem.hpp"
#include "Engine/Physics/Physics2D.hpp"
#include "Engine/Physics/Rigidbody2D.hpp"
#include "Engine/Physics/Collider2D.hpp"
#include "Engine/Audio/SoundManager.hpp"
#include "Engine/Core/GameObject.hpp"

namespace Game
{


    void SodaCanItem::Use(Engine::GameObject *user)
    {
        Engine::SoundManager::PlaySFX("soda_spray");
        float pushRadius = 8.0f;
        float pushStrength = 25.0f;

        Engine::Vector3 userPos = user->transform->position;
        auto colliders = Engine::Physics2D::OverlapCircleAll(userPos, pushRadius, 0.0f, 0.0f);

        for (auto *col : colliders)
        {
            if (col->gameObject == user)
                continue;

            auto rb = col->gameObject->GetComponent<Engine::Rigidbody2D>();
            if (rb && rb->bodyType == Engine::RigidbodyType2D::Dynamic)
            {
                Engine::Vector3 dir = col->GetCenter() - userPos;
                float dist = dir.Magnitude();
                if (dist > 0.001f)
                {
                    dir = Engine::Vector3(dir.x / dist, dir.y / dist);
                    rb->AddImpulse(dir * pushStrength);
                }
            }
        }

        Engine::GameObject::Destroy(gameObject);
    }

}