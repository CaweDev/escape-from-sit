#include "CCTVCamera.hpp"
#include "Engine/Core/GameObject.hpp"
#include <cmath>
#include <iostream>

namespace Game {

	static float DegToRad(float d) { return d * 3.1415926f / 180.0f; }

	bool CCTVCamera::InFov(float dx, float dy) const {
		float camRad = DegToRad(transform->rotation);
		float fx = std::cos(camRad);
		float fy = std::sin(camRad);

		float len = std::sqrt(dx * dx + dy * dy);
		if (len <= 0.00001f) return true;

		float tx = dx / len;
		float ty = dy / len;

		float dot = fx * tx + fy * ty;
		float halfFovRad = DegToRad(coneWidth * 0.5f);
		float cosHalfFov = std::cos(halfFovRad);
		return dot >= cosHalfFov;
	}

	bool CCTVCamera::RaycastTileLOS(Engine::Vector2 from, Engine::Vector2 to) const {
		float dx = to.x - from.x;
		float dy = to.y - from.y;
		float dist = std::sqrt(dx * dx + dy * dy);
		if (dist <= 0.00001f) return true;

		int steps = (int)(dist * rayStepPerUnit);
		if (steps < 1) steps = 1;

		float stepX = dx / (float)steps;
		float stepY = dy / (float)steps;

		float x = from.x;
		float y = from.y;

		for (int i = 0; i < steps; ++i) {
			if (!tile->IsWalkable(x, y)) return false;
			x += stepX;
			y += stepY;
		}
		return true;
	}

	bool CCTVCamera::HasLineOfSighttoPlayer() const {
		Engine::Vector2 camPos(transform->position.x, transform->position.y);
		Engine::Vector2 playerPos(tf->position.x, tf->position.y);
		return RaycastTileLOS(camPos, playerPos);
	}


	Engine::Vector2 CCTVCamera::RaycastToWall(Engine::Vector2 from, Engine::Vector2 dir, float maxDist) const {
		float dist = maxDist;
		int steps = (int)(dist * rayStepPerUnit);
		if (steps < 1) steps = 1;

		float stepX = (dir.x * dist) / (float)steps;
		float stepY = (dir.y * dist) / (float)steps;

		float x = from.x;
		float y = from.y;

		Engine::Vector2 lastGood = from;

		for (int i = 0; i < steps; ++i) {
			if (!tile->IsWalkable(x, y)) {
				return lastGood; 
			}
			lastGood = Engine::Vector2(x, y);
			x += stepX;
			y += stepY;
		}
		return Engine::Vector2(x, y);
	}

	
	void CCTVCamera::BuildVisionPolygon() {
		m_visionPoly.clear();
		if (!tile || !transform) return;
		if (!showVision) return;

		Engine::Vector2 camPos(transform->position.x, transform->position.y);

		float range = (maxRange > 0.0f) ? maxRange : detectRadius;
		float center = DegToRad(transform->rotation);
		float half = DegToRad(coneWidth * 0.5f);

		int rays = rayCount;
		if (rays < 6) rays = 6;

		for (int i = 0; i <= rays; ++i) {
			float t = (float)i / (float)rays;     
			float ang = (center - half) + (2.0f * half * t);

			Engine::Vector2 dir(std::cos(ang), std::sin(ang));
			Engine::Vector2 hit = RaycastToWall(camPos, dir, range);
			m_visionPoly.push_back(hit);
		}
	}


	bool CCTVCamera::IsTileVisible(int tx, int ty) const {
		if (!tile || !transform) return false;

		Engine::Vector2 camPos(transform->position.x, transform->position.y);
		Engine::Vector2 p((float)tx + 0.5f, (float)ty + 0.5f);

		float dx = p.x - camPos.x;
		float dy = p.y - camPos.y;

		float range = (maxRange > 0.0f) ? maxRange : detectRadius;
		float d2 = dx * dx + dy * dy;
		if (d2 > range * range) return false;

		if (!InFov(dx, dy)) return false;

		return RaycastTileLOS(camPos, p);
	}

	void CCTVCamera::Update() {
		if (!tile || !tf || !transform) return;

		BuildVisionPolygon();

		float distanceX = tf->position.x - transform->position.x;
		float distanceY = tf->position.y - transform->position.y;

		float distSq = distanceX * distanceX + distanceY * distanceY;
		float range = (maxRange > 0.0f) ? maxRange : detectRadius;
		if (distSq > range * range) return;

		if (!InFov(distanceX, distanceY)) return;

		if (needLOS && !HasLineOfSighttoPlayer()) return;

		RecordLastSeen();
		std::cout << "\n[CCTV " << id << "] Player spotted at Tile("
			<< (int)lastSeenTile.x << "," << (int)lastSeenTile.y
			<< ") World("
			<< lastSeenWorld.x << "," << lastSeenWorld.y
			<< ")\n";
	}

	void CCTVCamera::RecordLastSeen() {
		int tileX = (int)tf->position.x;
		int tileY = (int)tf->position.y;

		hasLastSeen = true;
		lastSeenTile = Engine::Vector2((float)tileX, (float)tileY);
		lastSeenWorld = Engine::Vector2((float)tileX + 0.5f, (float)tileY + 0.5f);
	}

}