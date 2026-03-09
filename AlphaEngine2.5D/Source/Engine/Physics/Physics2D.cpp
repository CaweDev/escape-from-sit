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
#include "../TileMap/Door.hpp"
#include "../Math/Mathf.hpp"
#include "../Graphics/Camera.hpp"
#include "AEEngine.h"
#include <algorithm>
#include <cmath>

namespace Engine {

// Static member initialization
f32 Physics2D::gravity = 9.81f;
bool Physics2D::debugDrawEnabled = false;

TileMap* Physics2D::tileMap = nullptr;
std::vector<Collider2D*> Physics2D::colliders;
std::vector<Rigidbody2D*> Physics2D::rigidbodies;

void Physics2D::FixedUpdate() {
    f32 dt = Time::fixedDeltaTime;
	if (dt <= EPSILON) return; // Avoid division by zero and unnecessary calculations
    
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
            if (tileMapCollider && tileMapCollider->enabled && tileMapCollider->tileMap && tileMapCollider->gameObject->active) {
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
                if (!other || other == collider || !other->enabled || !other->gameObject->active) continue;
                if (other->gameObject == rb->gameObject) continue;
                
                // Layer-based collision filtering
                if (!collider->CanCollideWith(other)) continue;
                
                if (collider->Overlaps(other)) {
                    if (other->isTrigger || collider->isTrigger) {
                        // Trigger callbacks
                        collider->OnTriggerStay2D(other);
                        other->OnTriggerStay2D(collider);
                    } else {
                        // Debug: log collision detection
                        // std::cout << "[Physics2D] Collision: " << rb->gameObject->name << " <-> " << other->gameObject->name << std::endl;
                        
                        // Collision response
                        ResolveCollision(rb, other);
                    }
                }
            }
            
            // Re-validate position against tilemap after collider-to-collider resolution
            // This prevents being pushed into walls by other entities
            Vector2 pushedPos(rb->transform->position.x, rb->transform->position.y);
            bool needsTileMapCheck = (pushedPos.x != newPos.x || pushedPos.y != newPos.y);
            
            if (needsTileMapCheck) {
                bool validPosition = true;
                
                // Check if pushed position is inside a wall
                for (Collider2D* tmCollider : colliders) {
                    TileMapCollider2D* tileMapCollider = dynamic_cast<TileMapCollider2D*>(tmCollider);
                    if (tileMapCollider && tileMapCollider->enabled && tileMapCollider->tileMap) {
                        // Check all four corners of the collision radius
                        if (!tileMapCollider->IsWalkable(pushedPos.x - radius, pushedPos.y - radius) ||
                            !tileMapCollider->IsWalkable(pushedPos.x + radius, pushedPos.y - radius) ||
                            !tileMapCollider->IsWalkable(pushedPos.x - radius, pushedPos.y + radius) ||
                            !tileMapCollider->IsWalkable(pushedPos.x + radius, pushedPos.y + radius)) {
                            validPosition = false;
                        }
                        break;
                    }
                }
                
                // Fallback to static tileMap if no TileMapCollider2D
                if (validPosition && !foundTileMapCollider && tileMap) {
                    if (!tileMap->IsWalkable(pushedPos.x - radius, pushedPos.y - radius) ||
                        !tileMap->IsWalkable(pushedPos.x + radius, pushedPos.y - radius) ||
                        !tileMap->IsWalkable(pushedPos.x - radius, pushedPos.y + radius) ||
                        !tileMap->IsWalkable(pushedPos.x + radius, pushedPos.y + radius)) {
                        validPosition = false;
                    }
                }
                
                // If pushed into a wall, revert to the tilemap-resolved position
                if (!validPosition) {
                    rb->transform->position.x = newPos.x;
                    rb->transform->position.y = newPos.y;
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

void Physics2D::DrawDebugView() {
    if (!debugDrawEnabled) return;
    if (colliders.empty()) return;
    if (!Camera::main || !Camera::main->transform) return;

    int screenWidth  = AEGfxGetWindowWidth();
    int screenHeight = AEGfxGetWindowHeight();
    if (screenWidth <= 0 || screenHeight <= 0) return;

    Transform* camTransform = Camera::main->transform;
    f32 cx       = camTransform->position.x;
    f32 cy       = camTransform->position.y;
    f32 camZ     = camTransform->position.z;
    f32 angleRad = camTransform->rotation * 3.14159f / 180.0f;
    f32 pitch    = Camera::main->pitch;
    int horizon  = screenHeight / 2 + (int)pitch;

    f32 fovRad      = Camera::main->fov * 3.14159f / 180.0f;
    f32 planeLength = tanf(fovRad / 2.0f);

    f32 dirX   = cosf(angleRad);
    f32 dirY   = sinf(angleRad);
    f32 planeX = dirY * planeLength;
    f32 planeY = -dirX * planeLength;

    f32 invDet = 1.0f / (planeX * dirY - dirX * planeY);
    f32 halfW  = screenWidth  * 0.5f;
    f32 halfH  = screenHeight * 0.5f;

    static constexpr f32 WORLD_HEIGHT  = 2.0f;
    static constexpr f32 NEAR_PLANE    = 0.2f; // raised to reduce near-plane projection blow-up
    static constexpr u32 COLOR_MAGENTA = 0xFFFF00FF;
    static constexpr u32 COLOR_WHITE   = 0xFFFFFFFF;

    // Projects a single world point into AE screen-space.
    // ty is the camera-depth; clamped to NEAR_PLANE on entry by AddLine.
    auto ProjectPoint = [&](f32 tx, f32 ty, f32 tz) -> std::pair<f32, f32> {
        f32 projH = (f32)screenWidth / (ty * planeLength * 2.0f);
        f32 sx    = halfW * (1.0f + tx / ty) - halfW;
        f32 sy    = halfH - ((f32)horizon + (camZ - tz) * projH);
        // Clamp sy to avoid lines streaking off screen when ty is very small.
        f32 syMax = halfH + (f32)screenHeight;
        f32 syMin = -(f32)screenHeight;
        if (sy > syMax) sy = syMax;
        if (sy < syMin) sy = syMin;
        return { sx, sy };
    };

    // Adds a perspective-projected 3D line segment with near-plane clipping.
    auto AddLine = [&](f32 wx1, f32 wy1, f32 wz1,
                       f32 wx2, f32 wy2, f32 wz2, u32 col) {
        f32 dx1 = wx1 - cx, dy1 = wy1 - cy;
        f32 tx1 = invDet * ( dirY * dx1 - dirX  * dy1);
        f32 ty1 = invDet * (-planeY * dx1 + planeX * dy1);
        f32 tz1 = wz1 / WORLD_HEIGHT;

        f32 dx2 = wx2 - cx, dy2 = wy2 - cy;
        f32 tx2 = invDet * ( dirY * dx2 - dirX  * dy2);
        f32 ty2 = invDet * (-planeY * dx2 + planeX * dy2);
        f32 tz2 = wz2 / WORLD_HEIGHT;

        if (ty1 < NEAR_PLANE && ty2 < NEAR_PLANE) return; // fully behind

        // Clip endpoint that is behind the near plane.
        if (ty1 < NEAR_PLANE || ty2 < NEAR_PLANE) {
            f32 t = (NEAR_PLANE - ty1) / (ty2 - ty1);
            if (ty1 < NEAR_PLANE) {
                tx1 = tx1 + t * (tx2 - tx1);
                tz1 = tz1 + t * (tz2 - tz1);
                ty1 = NEAR_PLANE;
            } else {
                tx2 = tx1 + t * (tx2 - tx1);
                tz2 = tz1 + t * (tz2 - tz1);
                ty2 = NEAR_PLANE;
            }
        }

        auto [sx1, sy1] = ProjectPoint(tx1, ty1, tz1);
        auto [sx2, sy2] = ProjectPoint(tx2, ty2, tz2);

        AEGfxVertexAdd(sx1, sy1, col, 0, 0);
        AEGfxVertexAdd(sx2, sy2, col, 0, 0);
    };

    // Draws all 12 edges of an AABB wireframe box.
    auto AddBox = [&](f32 x0, f32 y0, f32 x1, f32 y1, f32 hMin, f32 hMax, u32 col) -> int {
        AddLine(x0, y0, hMax, x1, y0, hMax, col);
        AddLine(x1, y0, hMax, x1, y1, hMax, col);
        AddLine(x1, y1, hMax, x0, y1, hMax, col);
        AddLine(x0, y1, hMax, x0, y0, hMax, col);
        AddLine(x0, y0, hMin, x1, y0, hMin, col);
        AddLine(x1, y0, hMin, x1, y1, hMin, col);
        AddLine(x1, y1, hMin, x0, y1, hMin, col);
        AddLine(x0, y1, hMin, x0, y0, hMin, col);
        AddLine(x0, y0, hMin, x0, y0, hMax, col);
        AddLine(x1, y0, hMin, x1, y0, hMax, col);
        AddLine(x1, y1, hMin, x1, y1, hMax, col);
        AddLine(x0, y1, hMin, x0, y1, hMax, col);
        return 12;
    };

    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetTransparency(1.0f);
    AEMtx33 identity;
    AEMtx33Identity(&identity);
    AEGfxSetTransform(identity.m);

    static constexpr int CIRCLE_SEGMENTS    = 16;
    static constexpr int MAX_LINES_PER_BATCH = 3000;

    AEGfxMeshStart();
    int lineCount = 0;

    auto Flush = [&]() {
        AEGfxVertexList* mesh = AEGfxMeshEnd();
        if (mesh) {
            if (lineCount > 0) AEGfxMeshDraw(mesh, AE_GFX_MDM_LINES);
            AEGfxMeshFree(mesh);
        }
        AEGfxMeshStart();
        lineCount = 0;
    };

    for (Collider2D* col : colliders) {
        if (!col || !col->gameObject || !col->gameObject->active) continue;
        if (col->isTrigger) continue;

        f32 hMin = col->minHeight;
        f32 hMax = col->maxHeight;

        // TileMapCollider2D: one white box per wall tile.
        TileMapCollider2D* tmc = dynamic_cast<TileMapCollider2D*>(col);
        if (tmc && tmc->tileMap) {
            TileMap* tm = tmc->tileMap;
            for (int ty = 0; ty < tm->height; ++ty) {
                for (int tx = 0; tx < tm->width; ++tx) {
                    if (!tm->IsWall(tx, ty)) continue;
                    f32 x0 = static_cast<f32>(tx);
                    f32 y0 = static_cast<f32>(ty);
                    lineCount += AddBox(x0, y0, x0 + 1.0f, y0 + 1.0f, hMin, hMax, COLOR_WHITE);
                    if (lineCount >= MAX_LINES_PER_BATCH) Flush();
                }
            }
            continue;
        }

        // Circle collider: magenta cylinder (top ring + bottom ring + vertical posts).
        CircleCollider2D* circle = dynamic_cast<CircleCollider2D*>(col);
        if (circle) {
            Vector2 center = circle->GetCenter();
            f32 r = circle->GetScaledRadius();
            for (int s = 0; s < CIRCLE_SEGMENTS; ++s) {
                f32 a0  = (f32)s       / (f32)CIRCLE_SEGMENTS * 2.0f * 3.14159f;
                f32 a1  = (f32)(s + 1) / (f32)CIRCLE_SEGMENTS * 2.0f * 3.14159f;
                f32 px0 = center.x + r * cosf(a0), py0 = center.y + r * sinf(a0);
                f32 px1 = center.x + r * cosf(a1), py1 = center.y + r * sinf(a1);
                AddLine(px0, py0, hMax, px1, py1, hMax, COLOR_MAGENTA);
                AddLine(px0, py0, hMin, px1, py1, hMin, COLOR_MAGENTA);
                AddLine(px0, py0, hMin, px0, py0, hMax, COLOR_MAGENTA);
            }
            lineCount += CIRCLE_SEGMENTS * 3;
        } else {
            // Box collider (doors, entities, etc.): magenta AABB.
            Bounds2D b = col->GetBounds();
            lineCount += AddBox(b.min.x, b.min.y, b.max.x, b.max.y, hMin, hMax, COLOR_MAGENTA);
        }

        if (lineCount >= MAX_LINES_PER_BATCH) Flush();
    }

    AEGfxVertexList* mesh = AEGfxMeshEnd();
    if (mesh) {
        if (lineCount > 0) AEGfxMeshDraw(mesh, AE_GFX_MDM_LINES);
        AEGfxMeshFree(mesh);
    }
}

} // namespace Engine
