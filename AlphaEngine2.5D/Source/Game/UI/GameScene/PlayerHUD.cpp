/**
 * @file PlayerHUD.cpp
 * @brief Implementation of the PlayerHUD component
 */

#include "PlayerHUD.hpp"
#include "Engine/UI/UIManager.hpp"
#include "Engine/Input/Input.hpp"

namespace Game
{
    PlayerHUD::~PlayerHUD()
    {
        // Canvas and its children are managed by UIManager
    }

    void PlayerHUD::Awake()
    {
        // Create a dedicated canvas for the HUD
        m_canvas = UI::UIManager::GetInstance()->CreateCanvas();
        m_canvas->SetReferenceResolution(1280.0f, 720.0f);
        m_canvas->SetScaleMode(UI::CanvasScaleMode::ScaleWithScreenSize);
        m_canvas->SetSortOrder(100);

        // Create minimap in bottom-left corner
        m_minimap = m_canvas->CreateChild<UI::Minimap>();
        m_minimap->name = "Minimap";
        m_minimap->rectTransform.anchorMin = { 0.0f, 0.0f };
        m_minimap->rectTransform.anchorMax = { 0.0f, 0.0f };
        m_minimap->rectTransform.pivot = { 0.0f, 0.0f };
        m_minimap->rectTransform.anchoredPosition = { m_minimapMargin, m_minimapMargin };
        m_minimap->rectTransform.sizeDelta = { m_minimapWidth, m_minimapHeight };

        // Create full map centered on screen (hidden by default)
        m_map = m_canvas->CreateChild<UI::Map>();
        m_map->name = "FullMap";
        m_map->rectTransform.anchorMin = { 0.5f, 0.5f };
        m_map->rectTransform.anchorMax = { 0.5f, 0.5f };
        m_map->rectTransform.pivot = { 0.5f, 0.5f };
        m_map->rectTransform.anchoredPosition = { 0.0f, 0.0f };
        m_map->rectTransform.sizeDelta = { m_mapWidth, m_mapHeight };
        m_map->visible = false;

     
        if (m_tileMap) SetTileMap(m_tileMap);
        if (m_playerTransform) SetPlayerTransform(m_playerTransform);

        for (auto* tf : m_enemyTransforms)
        {
            if (m_minimap) m_minimap->AddEnemyTransform(tf);
        }
		m_map->SetRenderOrder(1000000); 
		//m_map->wallColor = Engine::Color(0.f, 0.f, 0.f, 1.f); 
		m_map->SetBorderColor(Engine::Color(0.f, 0.f, 0.f, 1.f)); // Set alpha to 0 if using texture
		//m_map->SetTexture(Engine::AssetManager::GetTexture("Assets/border.png"));

        // If we have a transform, use it as player transform by default
        if (transform && !m_playerTransform)
        {
            SetPlayerTransform(transform);
        }
    }

    void PlayerHUD::Update()
    {
        // Toggle map with M key
        if (Engine::Input::GetKeyDown(Engine::KeyCode::M))
        {
            ToggleMap();
        }
    }

    void PlayerHUD::OnDestroy()
    {
        if (m_canvas)
        {
            UI::UIManager::GetInstance()->DestroyCanvas(m_canvas);
            m_canvas = nullptr;
            m_minimap = nullptr;
            m_map = nullptr;
        }
    }

    void PlayerHUD::SetTileMap(Engine::TileMap* tileMap)
    {
        m_tileMap = tileMap;

        if (m_minimap)
        {
            m_minimap->SetTileMap(tileMap);
        }

        if (m_map)
        {
            m_map->SetTileMap(tileMap);
        }
    }

    void PlayerHUD::SetPlayerTransform(Engine::Transform* playerTransform)
    {
        m_playerTransform = playerTransform;

        if (m_minimap)
        {
            m_minimap->SetPlayerTransform(playerTransform);
        }

        if (m_map)
        {
            m_map->SetPlayerTransform(playerTransform);
        }
    }

    void PlayerHUD::AddEnemyTransform(Engine::Transform* enemyTransform)
    {
        if (!enemyTransform) return;

     
        m_enemyTransforms.push_back(enemyTransform);

     
        if (m_minimap)
        {
            m_minimap->AddEnemyTransform(enemyTransform);
        }
        if (m_map) {
            m_map->AddEnemyTransform(enemyTransform);
        }
    }

    void PlayerHUD::ToggleMap()
    {
        m_mapVisible = !m_mapVisible;

        if (m_map)
        {
            m_map->visible = m_mapVisible;
        }
    }

    void PlayerHUD::ShowMap()
    {
        m_mapVisible = true;

        if (m_map)
        {
            m_map->visible = true;
        }
    }

    void PlayerHUD::HideMap()
    {
        m_mapVisible = false;

        if (m_map)
        {
            m_map->visible = false;
        }
    }

} // namespace Game
