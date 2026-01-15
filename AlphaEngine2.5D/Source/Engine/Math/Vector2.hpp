/**
 * @file Vector2.hpp
 * @brief 2D Vector class with Unity-like API
 */
#pragma once

#include "AEEngine.h"
#include <cmath>

namespace Engine {

/**
 * @class Vector2
 * @brief Represents a 2D vector with x and y components
 */
struct Vector2 {
    f32 x, y;

    // Constructors
    Vector2() : x(0), y(0) {}
    Vector2(f32 x, f32 y) : x(x), y(y) {}
    Vector2(const AEVec2& v) : x(v.x), y(v.y) {}

    // Static properties
    static Vector2 zero() { return Vector2(0, 0); }
    static Vector2 one() { return Vector2(1, 1); }
    static Vector2 up() { return Vector2(0, 1); }
    static Vector2 down() { return Vector2(0, -1); }
    static Vector2 left() { return Vector2(-1, 0); }
    static Vector2 right() { return Vector2(1, 0); }

    // Operators
    Vector2 operator+(const Vector2& other) const { return Vector2(x + other.x, y + other.y); }
    Vector2 operator-(const Vector2& other) const { return Vector2(x - other.x, y - other.y); }
    Vector2 operator*(f32 scalar) const { return Vector2(x * scalar, y * scalar); }
    Vector2 operator/(f32 scalar) const { return Vector2(x / scalar, y / scalar); }
    Vector2 operator-() const { return Vector2(-x, -y); }
    
    Vector2& operator+=(const Vector2& other) { x += other.x; y += other.y; return *this; }
    Vector2& operator-=(const Vector2& other) { x -= other.x; y -= other.y; return *this; }
    Vector2& operator*=(f32 scalar) { x *= scalar; y *= scalar; return *this; }
    Vector2& operator/=(f32 scalar) { x /= scalar; y /= scalar; return *this; }
    
    bool operator==(const Vector2& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Vector2& other) const { return !(*this == other); }

    // Methods
    f32 Magnitude() const { return std::sqrt(x * x + y * y); }
    f32 SqrMagnitude() const { return x * x + y * y; }
    
    Vector2 Normalized() const {
        f32 mag = Magnitude();
        if (mag > 0) return *this / mag;
        return zero();
    }
    
    void Normalize() {
        f32 mag = Magnitude();
        if (mag > 0) { x /= mag; y /= mag; }
    }

    // Static methods
    static f32 Dot(const Vector2& a, const Vector2& b) {
        return a.x * b.x + a.y * b.y;
    }
    
    static f32 Distance(const Vector2& a, const Vector2& b) {
        return (a - b).Magnitude();
    }
    
    static Vector2 Lerp(const Vector2& a, const Vector2& b, f32 t) {
        t = (t < 0) ? 0 : (t > 1) ? 1 : t;
        return Vector2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
    }
    
    static Vector2 LerpUnclamped(const Vector2& a, const Vector2& b, f32 t) {
        return Vector2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
    }

    // Conversion to Alpha Engine type
    AEVec2 ToAEVec2() const {
        AEVec2 v;
        v.x = x;
        v.y = y;
        return v;
    }
};

// Scalar * Vector2
inline Vector2 operator*(f32 scalar, const Vector2& v) {
    return Vector2(v.x * scalar, v.y * scalar);
}

} // namespace Engine
