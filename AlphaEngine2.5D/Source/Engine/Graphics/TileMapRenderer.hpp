/**
 * @file TileMapRenderer.hpp
 * @brief Component for rendering TileMaps using raycasting
 */
#pragma once

#include "Engine/TileMap/TileMap.hpp"
#include "Engine/Graphics/IRenderer.hpp"
#include "Engine/Graphics/Color.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "AEEngine.h"
#include <cmath>
#include <iostream>

namespace Engine {

	/**
	 * @class TileMapRenderer
	 * @brief Renders walls using DDA raycasting algorithm
	 */
	class TileMapRenderer : public IRenderer {
	public:
		REQUIRE_COMPONENT(TileMap);

		/** Field of view in degrees */
		f32 fov = 75.0f;

		/** Wall colors for different sides */
		Color wallColorNS = Color(0.8f, 0.2f, 0.2f, 1.0f);
		Color wallColorEW = Color(0.6f, 0.15f, 0.15f, 1.0f);

		/** Sky and floor colors */
		Color ceilingColor = Color(0.2f, 0.2f, 0.4f, 1.0f);
		Color floorColor = Color(0.3f, 0.3f, 0.3f, 1.0f);

		TileMapRenderer();

		~TileMapRenderer();

		/** Render the scene from current transform perspective */
		void Render() override;

	private:
		AEGfxVertexList* columnMesh = nullptr;

		void CreateColumnMesh();

		void DrawColumn(f32 x, f32 y, f32 h, const Color& color, int sw, int sh);

		void DrawRect(f32 x, f32 y, f32 w, f32 h, const Color& color, int sw, int sh);
	};

} // namespace Engine
