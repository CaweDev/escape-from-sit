/**
 * @file SpriteRenderer.hpp
 * @brief Component for rendering sprites
 */
#pragma once

#include "../Core/Component.hpp"
#include "../Core/Transform.hpp"
#include "IRenderer.hpp"
#include "Sprite.hpp"
#include "Color.hpp"
#include "../Raycasting/ProjectionHelper.hpp"
#include "Camera.hpp"
#include "AEEngine.h"
#include <memory>
#include <Engine/Core/Application.hpp>

namespace Engine {

/**
 * @class SpriteRenderer
 * @brief Component that renders a sprite at the GameObject's transform
 */
    class SpriteRenderer : public IRenderer {
    public:
        /** The sprite to render */
        Sprite* sprite;

        /** Color tint multiplier */
        Color color = Color::white();

        /** Sorting order for z-depth (higher = rendered later/on top) */
        int sortingOrder = 0;

        /** Flip sprite horizontally */
        bool flipX = false;

        /** Flip sprite vertically */
        bool flipY = false;

        /** If true, renders as a 2.5D billboard in raycast space */
        bool billboard = false;

        SpriteRenderer();

        /** Set sprite from shared pointer */
        void SetSprite(std::shared_ptr<Sprite> newSprite);

        /** Set sprite from shared pointer */
        void SetSprite(Sprite* newSprite);

        /** Render the sprite (called by renderer system) */
        void Render() override;

        /** Get sorting key for depth sorting (combines z position and sortingOrder) */
        f32 GetSortingKey() const override;

    private:
        void AddStripToMesh(int x1, int x2, f32 spriteStart, f32 spriteWidth, f32 yTop, f32 yBottom);
    };
} // namespace Engine
