/**
 * @file Door.hpp
 * @brief Door component for interactive doors in the tilemap
 */
#pragma once

#include "../Core/Component.hpp"
#include "../Math/Vector2.hpp"
#include "AEEngine.h"
#include <unordered_set>
#include <unordered_map>
#include <cstdint>


namespace Engine {

// Forward declarations
class BoxCollider2D;
class Interactable;

/**
 * @enum DoorState
 * @brief Current state of the door
 */
enum class DoorState {
    Closed,   // Door is fully closed
    Opening,  // Door is animating open
    Open,     // Door is fully open
    Closing   // Door is animating closed
};

/**
 * @enum DoorFacing
 * @brief Direction the door faces (determines collision box orientation)
 */
enum class DoorFacing {
    NorthSouth,  // Door faces N/S (blocks E/W movement when closed)
    EastWest     // Door faces E/W (blocks N/S movement when closed)
};

/**
 * @class Door
 * @brief Interactive door component with animation support
 * 
 * Doors can be opened/closed with smooth animation. When open, they allow
 * passage through. Designed to work with the Interactable system.
 */
class Door : public Component {
public:
    /** Tile position in tilemap coordinates */
    int tileX = 0;
    int tileY = 0;
    
    /** Door type (10-13, used for texture lookup) */
    int doorType = 10;
    
    /** Facing direction */
    DoorFacing facing = DoorFacing::NorthSouth;
    
    /** Current door state */
    DoorState state = DoorState::Closed;
    
    /** Animation progress (0.0 = closed, 1.0 = open) */
    f32 openAmount = 0.0f;
    
    /** Animation speed (full open/close per second) */
    f32 animationSpeed = 2.0f;
    
    /** Auto-close delay in seconds (0 = never auto-close) */
    f32 autoCloseDelay = 0.0f;
    
  

    Door() = default;
    virtual ~Door() = default;
    
    // Lifecycle
    void Awake() override;
    void Update() override;
    
    // Door control
    
    /** Start opening the door */
    void Open();
    
    /** Start closing the door */
    void Close();
    
    /** Toggle between open and closed */
    void Toggle();
    
    // State queries
    
    /** Check if door allows passage (open enough to walk through) */
    bool IsPassable() const;
    
    /** Check if door is fully open */
    bool IsFullyOpen() const { return state == DoorState::Open; }
    
    /** Check if door is fully closed */
    bool IsFullyClosed() const { return state == DoorState::Closed; }
    
    /** Check if door is currently animating */
    bool IsAnimating() const { return state == DoorState::Opening || state == DoorState::Closing; }
    
    /** Get world position (center of door tile) */
    Vector2 GetWorldPosition() const;
    
    // Integration helpers
    
    /**
     * @brief Set up an Interactable to control this door
     * @param interactable The Interactable component to bind
     * @param openText Text shown when door is closed (default: "Open")
     * @param closeText Text shown when door is open (default: "Close")
     */
    void SetupInteractable(Interactable* interactable, 
                           const char* openText = "Open",
                           const char* closeText = "Close");
    
    /**
     * @brief Disable this door's interaction (for when another component takes over)
     * Used by ControlRoomDoor to prevent base Door from handling interactions
     */
    void DisableInteraction();

    /** Cached reference to collider (for enabling/disabling) */
    BoxCollider2D* collider = nullptr;


    void RequestOpen(const void* requester);
    void ReleaseOpen(const void* requester);
    bool IsHeldOpen() const {
        return !m_holders.empty();
    }
    static Door* FindAtTile(int tileX, int tileY);

private:
    
    /** Auto-close timer */
    f32 autoCloseTimer = 0.0f;
    
    /** Update collider enabled state based on passability */
    void UpdateCollider();
    
    /** Interactable text options for dynamic updates */
    const char* m_openText = "Open";
    const char* m_closeText = "Close";
    Interactable* m_interactable = nullptr;

    std::unordered_set<const void*>m_holders;


};

} // namespace Engine
