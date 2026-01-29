/**
 * @file Physics2D.cpp
 * @brief Implementation of Physics2D static physics manager
 */
#include "Physics2D.hpp"
#include "Collider2D.hpp"
#include "BoxCollider2D.hpp"
#include "CircleCollider2D.hpp"
#include "Rigidbody2D.hpp"
#include "TileMapCollider2D.hpp"
#include "../Core/Transform.hpp"
#include "../Core/Time.hpp"
#include "../Core/GameObject.hpp"
#include "../TileMap/TileMap.hpp"
#include "../Math/Mathf.hpp"
#include <algorithm>

namespace Engine {

// Static member initialization
f32 Physics2D::gravity = 9.81f;
f32 Physics2D::fixedDeltaTime = 0.02f;
TileMap* Physics2D::tileMap = nullptr;
std::vector<Collider2D*> Physics2D::colliders;
std::vector<Rigidbody2D*> Physics2D::rigidbodies;

void Physics2D::FixedUpdate() {
    f32 dt = Time::fixedDeltaTime;
    if (dt <= 0) dt = fixedDeltaTime;
    
    // Update all rigidbodies
    for (Rigidbody2D* rb : rigidbodies) {
        if (!rb || !rb->enabled || !rb->transform) continue;
        if (rb->bodyType == RigidbodyType2D::Static) continue;
        
        // Calculate movement
        Vector2 movement = rb->velocity * dt;
        
        // Apply tilemap collision
        Vector2 currentPos(rb->transform->position.x, rb->transform->position.y);
        Collider2D* collider = rb->GetCollider();
        f32 radius = 0.2f; // Default collision radius
        
        if (collider) {
            // Get radius from circle collider or box half-size
            CircleCollider2D* circle = dynamic_cast<CircleCollider2D*>(collider);
            if (circle) {
                radius = circle->GetScaledRadius();
            } else {
                BoxCollider2D* box = dynamic_cast<BoxCollider2D*>(collider);
                if (box) {
                    radius = Mathf::Min(box->size.x, box->size.y) * 0.5f;
                }
            }
        }
        
        // Find TileMapCollider2D and use it for tilemap collision
        Vector2 newPos = currentPos + movement;
        bool foundTileMapCollider = false;
        for (Collider2D* tmCollider : colliders) {
            TileMapCollider2D* tileMapCollider = dynamic_cast<TileMapCollider2D*>(tmCollider);
            if (tileMapCollider && tileMapCollider->enabled && tileMapCollider->tileMap) {
                newPos = tileMapCollider->ResolveMovement(currentPos, movement, radius);
                foundTileMapCollider = true;
                break;
            }
        }
        
        // Fallback to static tilemap if no TileMapCollider2D found
        if (!foundTileMapCollider && tileMap) {
            newPos = TileMapCollision(currentPos, movement, radius);
        }
        
        // Check collider-to-collider collisions
        if (collider) {
            // Temporarily move to new position
            rb->transform->position.x = newPos.x;
            rb->transform->position.y = newPos.y;
            
            for (Collider2D* other : colliders) {
                if (!other || other == collider || !other->enabled) continue;
                if (other->gameObject == rb->gameObject) continue;
                
                // Layer-based collision filtering
                if (!collider->CanCollideWith(other)) continue;
                
                if (collider->Overlaps(other)) {
                    if (other->isTrigger || collider->isTrigger) {
                        // Trigger callbacks
                        collider->OnTriggerStay2D(other);
                        other->OnTriggerStay2D(collider);
                    } else {
                        // Collision response
                        ResolveCollision(rb, other);
                    }
                }
            }
        } else {
            rb->transform->position.x = newPos.x;
            rb->transform->position.y = newPos.y;
        }
    }
}

void Physics2D::RegisterCollider(Collider2D* collider) {
    if (!collider) return;
    auto it = std::find(colliders.begin(), colliders.end(), collider);
    if (it == colliders.end()) {
        colliders.push_back(collider);
    }
}

void Physics2D::UnregisterCollider(Collider2D* collider) {
    colliders.erase(std::remove(colliders.begin(), colliders.end(), collider), colliders.end());
}

void Physics2D::RegisterRigidbody(Rigidbody2D* rb) {
    if (!rb) return;
    auto it = std::find(rigidbodies.begin(), rigidbodies.end(), rb);
    if (it == rigidbodies.end()) {
        rigidbodies.push_back(rb);
    }
}

void Physics2D::UnregisterRigidbody(Rigidbody2D* rb) {
    rigidbodies.erase(std::remove(rigidbodies.begin(), rigidbodies.end(), rb), rigidbodies.end());
}

bool Physics2D::IsWalkable(f32 x, f32 y) {
    if (!tileMap) return true;
    return tileMap->IsWalkable(x, y);
}

Vector2 Physics2D::TileMapCollision(const Vector2& position, const Vector2& movement, f32 radius) {
    if (!tileMap) {
        return position + movement;
    }
    
    Vector2 newPos = position;
    
    // Try X movement
    f32 nextX = position.x + movement.x;
    if (tileMap->IsWalkable(nextX - radius, position.y - radius) &&
        tileMap->IsWalkable(nextX + radius, position.y - radius) &&
        tileMap->IsWalkable(nextX - radius, position.y + radius) &&
        tileMap->IsWalkable(nextX + radius, position.y + radius)) {
        newPos.x = nextX;
    }
    
    // Try Y movement
    f32 nextY = position.y + movement.y;
    if (tileMap->IsWalkable(newPos.x - radius, nextY - radius) &&
        tileMap->IsWalkable(newPos.x + radius, nextY - radius) &&
        tileMap->IsWalkable(newPos.x - radius, nextY + radius) &&
        tileMap->IsWalkable(newPos.x + radius, nextY + radius)) {
        newPos.y = nextY;
    }
    
    return newPos;
}

void Physics2D::ResolveCollision(Rigidbody2D* rb, Collider2D* other) {
    if (!rb || !other || !rb->transform) return;
    
    Collider2D* rbCollider = rb->GetCollider();
    if (!rbCollider) return;
    
    // Get centers
    Vector2 rbCenter = rbCollider->GetCenter();
    Vector2 otherCenter = other->GetCenter();
    
    // Calculate push direction
    Vector2 pushDir = rbCenter - otherCenter;
    if (pushDir.SqrMagnitude() < 0.0001f) {
        pushDir = Vector2(1, 0); // Default push direction
    }
    pushDir.Normalize();
    
    // Calculate penetration depth
    Bounds2D rbBounds = rbCollider->GetBounds();
    Bounds2D otherBounds = other->GetBounds();
    
    f32 overlapX = Mathf::Min(rbBounds.max.x, otherBounds.max.x) - Mathf::Max(rbBounds.min.x, otherBounds.min.x);
    f32 overlapY = Mathf::Min(rbBounds.max.y, otherBounds.max.y) - Mathf::Max(rbBounds.min.y, otherBounds.min.y);
    
    // Push out along smallest overlap axis (hard resolution)
    if (overlapX < overlapY && overlapX > 0) {
        if (rbCenter.x < otherCenter.x) {
            rb->transform->position.x -= overlapX;
        } else {
            rb->transform->position.x += overlapX;
        }
        rb->velocity.x = 0;
    } else if (overlapY > 0) {
        if (rbCenter.y < otherCenter.y) {
            rb->transform->position.y -= overlapY;
        } else {
            rb->transform->position.y += overlapY;
        }
        rb->velocity.y = 0;
    }
    
    // Create collision info
    Collision2D collision;
    collision.collider = other;
    collision.gameObject = other->gameObject;
    collision.normal = pushDir;
    collision.penetration = Mathf::Min(overlapX, overlapY);
    
    // Call callbacks
    rbCollider->OnCollisionStay2D(collision);
}

RaycastHit2D Physics2D::Raycast(const Vector2& origin, const Vector2& direction, f32 maxDistance) {
    RaycastHit2D hit;
    f32 closestDist = maxDistance;
    
    Vector2 dir = direction.Normalized();
    
    for (Collider2D* collider : colliders) {
        if (!collider || !collider->enabled || collider->isTrigger) continue;
        
        Bounds2D bounds = collider->GetBounds();
        
        // Simple AABB ray intersection
        f32 tmin = 0.0f;
        f32 tmax = maxDistance;
        
        // X slab
        if (Mathf::Abs(dir.x) > Mathf::Epsilon) {
            f32 t1 = (bounds.min.x - origin.x) / dir.x;
            f32 t2 = (bounds.max.x - origin.x) / dir.x;
            if (t1 > t2) std::swap(t1, t2);
            tmin = Mathf::Max(tmin, t1);
            tmax = Mathf::Min(tmax, t2);
        } else if (origin.x < bounds.min.x || origin.x > bounds.max.x) {
            continue;
        }
        
        // Y slab
        if (Mathf::Abs(dir.y) > Mathf::Epsilon) {
            f32 t1 = (bounds.min.y - origin.y) / dir.y;
            f32 t2 = (bounds.max.y - origin.y) / dir.y;
            if (t1 > t2) std::swap(t1, t2);
            tmin = Mathf::Max(tmin, t1);
            tmax = Mathf::Min(tmax, t2);
        } else if (origin.y < bounds.min.y || origin.y > bounds.max.y) {
            continue;
        }
        
        if (tmin <= tmax && tmin >= 0 && tmin < closestDist) {
            closestDist = tmin;
            hit.collider = collider;
            hit.point = origin + dir * tmin;
            hit.distance = tmin;
            
            // Calculate normal (simplified - use closest axis)
            Vector2 center = bounds.Center();
            Vector2 toHit = hit.point - center;
            if (Mathf::Abs(toHit.x) > Mathf::Abs(toHit.y)) {
                hit.normal = Vector2(Mathf::Sign(toHit.x), 0);
            } else {
                hit.normal = Vector2(0, Mathf::Sign(toHit.y));
            }
        }
    }
    
    return hit;
}

std::vector<RaycastHit2D> Physics2D::RaycastAll(const Vector2& origin, const Vector2& direction, f32 maxDistance) {
    std::vector<RaycastHit2D> hits;
    // Simplified: just return single hit for now
    RaycastHit2D hit = Raycast(origin, direction, maxDistance);
    if (hit) hits.push_back(hit);
    return hits;
}

Collider2D* Physics2D::OverlapCircle(const Vector2& center, f32 radius, f32 height, f32 heightPos) {
    for (Collider2D* collider : colliders) {
        if (!collider || !collider->enabled) continue;
        
        // Check height overlap
        if (!collider->HeightContains(heightPos, height)) continue;
        
        // Check 2D overlap
        Bounds2D bounds = collider->GetBounds();
        Vector2 closest;
        closest.x = Mathf::Clamp(center.x, bounds.min.x, bounds.max.x);
        closest.y = Mathf::Clamp(center.y, bounds.min.y, bounds.max.y);
        
        if (Vector2::Distance(center, closest) <= radius) {
            return collider;
        }
    }
    return nullptr;
}

std::vector<Collider2D*> Physics2D::OverlapCircleAll(const Vector2& center, f32 radius, f32 height, f32 heightPos) {
    std::vector<Collider2D*> result;
    for (Collider2D* collider : colliders) {
        if (!collider || !collider->enabled) continue;
        
        if (!collider->HeightContains(heightPos, height)) continue;
        
        Bounds2D bounds = collider->GetBounds();
        Vector2 closest;
        closest.x = Mathf::Clamp(center.x, bounds.min.x, bounds.max.x);
        closest.y = Mathf::Clamp(center.y, bounds.min.y, bounds.max.y);
        
        if (Vector2::Distance(center, closest) <= radius) {
            result.push_back(collider);
        }
    }
    return result;
}

Collider2D* Physics2D::OverlapBox(const Vector2& center, const Vector2& size, f32 height, f32 heightPos) {
    Bounds2D queryBounds;
    queryBounds.min = center - size * 0.5f;
    queryBounds.max = center + size * 0.5f;
    
    for (Collider2D* collider : colliders) {
        if (!collider || !collider->enabled) continue;
        
        if (!collider->HeightContains(heightPos, height)) continue;
        
        if (queryBounds.Overlaps(collider->GetBounds())) {
            return collider;
        }
    }
    return nullptr;
}

std::vector<Collider2D*> Physics2D::OverlapBoxAll(const Vector2& center, const Vector2& size, f32 height, f32 heightPos) {
    std::vector<Collider2D*> result;
    Bounds2D queryBounds;
    queryBounds.min = center - size * 0.5f;
    queryBounds.max = center + size * 0.5f;
    
    for (Collider2D* collider : colliders) {
        if (!collider || !collider->enabled) continue;
        
        if (!collider->HeightContains(heightPos, height)) continue;
        
        if (queryBounds.Overlaps(collider->GetBounds())) {
            result.push_back(collider);
        }
    }
    return result;
}

Vector2 Physics2D::MoveWithCollision(Rigidbody2D* rb, const Vector2& movement) {
    if (!rb || !rb->transform) return movement;
    
    Vector2 currentPos(rb->transform->position.x, rb->transform->position.y);
    Collider2D* collider = rb->GetCollider();
    f32 radius = 0.2f;
    
    if (collider) {
        CircleCollider2D* circle = dynamic_cast<CircleCollider2D*>(collider);
        if (circle) {
            radius = circle->GetScaledRadius();
        }
    }
    
    return TileMapCollision(currentPos, movement, radius);
}

} // namespace Engine
