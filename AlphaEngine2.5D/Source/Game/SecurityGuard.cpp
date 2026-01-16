#include "SecurityGuard.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"

#include <cmath>

void Game::SecurityGuard::Initialize()
{


}

void Game::SecurityGuard::Update()
{
    if (!transform) return;

    Engine::Vector2 target;
    if (goingtonextPoint) {
        target = pointB;
    }
    else {
        target = pointA;
    }
    Engine::Vector2 delta;
    delta.x = target.x - transform->position.x;
    delta.y = target.y - transform->position.y;
    float distanceSq = delta.x * delta.x + delta.y * delta.y;
    if (distanceSq <= arriveDistance * arriveDistance)
    {
        goingtonextPoint = !goingtonextPoint;
        return;
    }
    float length = std::sqrt(distanceSq);
    Engine::Vector2 dir(delta.x / length, delta.y / length);
    transform->position.x += dir.x * speed * Engine::Time::deltaTime;
    transform->position.y += dir.y * speed * Engine::Time::deltaTime;
}
