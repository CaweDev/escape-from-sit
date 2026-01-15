/**
 * @file Vector3.hpp
 * @brief 3D Vector class for 2.5D depth sorting
 */
#pragma once

#include "Vector2.hpp"
#include <cmath>

namespace Engine {

/**
 * @class Vector3
 * @brief Represents a 3D vector with x, y, z components (z used for depth/sorting)
 */
struct Vector3 {
    f32 x, y, z;

    // Constructors
    Vector3() : x(0), y(0), z(0) {}
    Vector3(f32 x, f32 y, f32 z = 0) : x(x), y(y), z(z) {}
    Vector3(const Vector2& v, f32 z = 0) : x(v.x), y(v.y), z(z) {}

    // Static properties
    static Vector3 zero() { return Vector3(0, 0, 0); }
    static Vector3 one() { return Vector3(1, 1, 1); }
    static Vector3 up() { return Vector3(0, 1, 0); }
    static Vector3 down() { return Vector3(0, -1, 0); }
    static Vector3 left() { return Vector3(-1, 0, 0); }
    static Vector3 right() { return Vector3(1, 0, 0); }
    static Vector3 forward() { return Vector3(0, 0, 1); }
    static Vector3 back() { return Vector3(0, 0, -1); }

    // Operators
    Vector3 operator+(const Vector3& other) const { return Vector3(x + other.x, y + other.y, z + other.z); }
    Vector3 operator-(const Vector3& other) const { return Vector3(x - other.x, y - other.y, z - other.z); }
    Vector3 operator*(f32 scalar) const { return Vector3(x * scalar, y * scalar, z * scalar); }
    Vector3 operator/(f32 scalar) const { return Vector3(x / scalar, y / scalar, z / scalar); }
    Vector3 operator-() const { return Vector3(-x, -y, -z); }
    
    Vector3& operator+=(const Vector3& other) { x += other.x; y += other.y; z += other.z; return *this; }
    Vector3& operator-=(const Vector3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    Vector3& operator*=(f32 scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
    Vector3& operator/=(f32 scalar) { x /= scalar; y /= scalar; z /= scalar; return *this; }
    
    bool operator==(const Vector3& other) const { return x == other.x && y == other.y && z == other.z; }
    bool operator!=(const Vector3& other) const { return !(*this == other); }

    // Methods
    f32 Magnitude() const { return std::sqrt(x * x + y * y + z * z); }
    f32 SqrMagnitude() const { return x * x + y * y + z * z; }
    
    Vector3 Normalized() const {
        f32 mag = Magnitude();
        if (mag > 0) return *this / mag;
        return zero();
    }
    
    void Normalize() {
        f32 mag = Magnitude();
        if (mag > 0) { x /= mag; y /= mag; z /= mag; }
    }

    // Conversion to Vector2 (drops z)
    Vector2 ToVector2() const { return Vector2(x, y); }

    // Static methods
    static f32 Dot(const Vector3& a, const Vector3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    
    static Vector3 Cross(const Vector3& a, const Vector3& b) {
        return Vector3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }
    
    static f32 Distance(const Vector3& a, const Vector3& b) {
        return (a - b).Magnitude();
    }
    
    static Vector3 Lerp(const Vector3& a, const Vector3& b, f32 t) {
        t = (t < 0) ? 0 : (t > 1) ? 1 : t;
        return Vector3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
    }
    
    static Vector3 LerpUnclamped(const Vector3& a, const Vector3& b, f32 t) {
        return Vector3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
    }
};

// Scalar * Vector3
inline Vector3 operator*(f32 scalar, const Vector3& v) {
    return Vector3(v.x * scalar, v.y * scalar, v.z * scalar);
}

} // namespace Engine
