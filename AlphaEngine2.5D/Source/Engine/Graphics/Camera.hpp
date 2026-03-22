/**
 * @file Camera.hpp
 * @brief Main camera for 2D rendering
 */
#pragma once

#include "Engine/Core/Component.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Color.hpp"
#include "AEEngine.h"
#include <string>

namespace Engine {

/**
 * @brief IConfig implementation that deserialises camera.json.
 */
struct CameraConfig : public IConfig
{
    Color   backgroundColor  = Color(0.2f, 0.2f, 0.2f, 1.0f);
    float   orthographicSize = 5.0f;
    float   fov              = 75.0f;
    float   pitch            = 0.0f;
    Color   fogColor         = Color(0.0f, 0.0f, 0.0f, 1.0f);
    float   fogDistance      = 20.0f;
    float   fogMinVisibility = 0.2f;
    float   fogMaxVisibility = 1.0f;
    float   fogPrecision     = 200.0f;
    bool    fogEnabled       = false;

    void FromJson(const Json::Value& j) override
    {
        if (j.isMember("backgroundColor")) {
            const auto& bg = j["backgroundColor"];
            backgroundColor = Color(bg.get("r", backgroundColor.r).asFloat(),
                                    bg.get("g", backgroundColor.g).asFloat(),
                                    bg.get("b", backgroundColor.b).asFloat(),
                                    bg.get("a", backgroundColor.a).asFloat());
        }
        orthographicSize = j.get("orthographicSize", orthographicSize).asFloat();
        fov              = j.get("fov",              fov).asFloat();
        pitch            = j.get("pitch",            pitch).asFloat();
        if (j.isMember("fogColor")) {
            const auto& fc = j["fogColor"];
            fogColor = Color(fc.get("r", fogColor.r).asFloat(),
                             fc.get("g", fogColor.g).asFloat(),
                             fc.get("b", fogColor.b).asFloat(),
                             fc.get("a", fogColor.a).asFloat());
        }
        fogDistance      = j.get("fogDistance",      fogDistance).asFloat();
        fogMinVisibility = j.get("fogMinVisibility", fogMinVisibility).asFloat();
        fogMaxVisibility = j.get("fogMaxVisibility", fogMaxVisibility).asFloat();
        fogPrecision     = j.get("fogPrecision",     fogPrecision).asFloat();
        fogEnabled       = j.get("fogEnabled",       fogEnabled).asBool();
    }
};

/**
 * @class Camera
 * @brief Component for camera control and viewport management
 */
class Camera : public Component {
public:
    /** Background clear color */
    Color backgroundColor = Color(0.2f, 0.2f, 0.2f, 1.0f);
    
    /** Orthographic size (half-height in world units) */
    f32 orthographicSize = 5.0f;
    
    f32 fov = 75.0f;

    /** Vertical rotation (pitch) for 2.5D dynamic horizon */
    f32 pitch = 0.0f;

    /** Fog settings for distance-based darkening (global, affects all renderers) */
    Color fogColor = Color(0.0f, 0.0f, 0.0f, 1.0f);  // Black by default
    f32 fogDistance = 20.0f;   // Distance at which fog reaches maximum
    f32 fogMinVisibility = 0.2f;  // Minimum visibility (prevents total blackout at distance)
    f32 fogMaxVisibility = 1.0f;  // Maximum visibility (caps brightness at close range)
    f32 fogPrecision = 200.0f;    // Precision for fog quantization (increments = 1.0 / fogPrecision)
    bool fogEnabled = false;  // Disabled by default

    /** Static reference to main camera */
    static Camera* main;

    /**
     * @brief Apply defaults from camera.json to every new Camera at Awake time.
     * Call once at startup. Falls back to built-in defaults if the file is missing.
     * @param configPath Path to camera.json.
     */
    static void Initialize(const std::string& configPath = "Assets/Config/Engine/camera.json");

    void Awake() override;
    
    void OnDestroy() override;

    /** Apply camera settings (called before rendering) */
    void Apply();
    
    /** Convert screen position to world position */
    Vector2 ScreenToWorldPoint(const Vector2& screenPos);
    
    /** Convert world position to screen position */
    Vector2 WorldToScreenPoint(const Vector2& worldPos);
    
    /** Get viewport width in world units */
    f32 GetViewportWidth() const;
    
    /** Get viewport height in world units */
    f32 GetViewportHeight() const;


};

} // namespace Engine
