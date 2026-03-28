#include "Item.hpp"

#include "Game/Inventory/Inventory.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Interaction/Interactable.hpp"
#include "Engine/Audio/SoundManager.hpp"
namespace Game
{

    void Item::Initialize()
    {
        interactable = gameObject->GetComponent<Engine::Interactable>();
        itemData = gameObject->GetComponent<Item>();

        interactable->onInteract = [this]()
        { this->OnCollect(); };
    }

    void Item::OnCollect()
    {
        auto playerObj = Engine::GameObject::Find("Player");
        if (!playerObj)
            return;

        auto inventory = playerObj->GetComponent<Inventory>();
        if (!inventory)
            return;

        if (inventory->AddItem(gameObject))
        {
            // Engine::SoundManager::PlaySFX("item_pickup");
        }
    }
}
