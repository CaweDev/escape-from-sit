/**
 * @file Vector3.cpp
 */
#include "Vector3.hpp"
#include "Vector2.hpp"
#include <cmath>

namespace Engine
{
    Vector3::Vector3() : x(0), y(0), z(0) {}
    Vector3::Vector3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}
    Vector3::Vector3(const Vector2 &v, f32 z) : x(v.x), y(v.y), z(z) {}

    
    
    Vector3 Vector3::zero() { return Vector3(0, 0, 0); }
    
    
    Vector3 Vector3::one() { return Vector3(1, 1, 1); }
    
    
    Vector3 Vector3::up() { return Vector3(0, 1, 0); }
    
    
    Vector3 Vector3::down() { return Vector3(0, -1, 0); }
    
    
    Vector3 Vector3::left() { return Vector3(-1, 0, 0); }
    
    
    Vector3 Vector3::right() { return Vector3(1, 0, 0); }
    
    
    Vector3 Vector3::forward() { return Vector3(0, 0, 1); }
    
    
    Vector3 Vector3::back() { return Vector3(0, 0, -1); }

    Vector3 Vector3::operator+(const Vector3 &other) const { return Vector3(x + other.x, y + other.y, z + other.z); }
    Vector3 Vector3::operator-(const Vector3 &other) const { return Vector3(x - other.x, y - other.y, z - other.z); }
    Vector3 Vector3::operator*(f32 scalar) const { return Vector3(x * scalar, y * scalar, z * scalar); }
    Vector3 Vector3::operator/(f32 scalar) const { return Vector3(x / scalar, y / scalar, z / scalar); }
    Vector3 Vector3::operator-() const { return Vector3(-x, -y, -z); }

    Vector3 &Vector3::operator+=(const Vector3 &other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
    Vector3 &Vector3::operator-=(const Vector3 &other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }
    Vector3 &Vector3::operator*=(f32 scalar)
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }
    Vector3 &Vector3::operator/=(f32 scalar)
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    bool Vector3::operator==(const Vector3 &other) const { return x == other.x && y == other.y && z == other.z; }
    bool Vector3::operator!=(const Vector3 &other) const { return !(*this == other); }

    
    
    f32 Vector3::Magnitude() const { return std::sqrt(x * x + y * y + z * z); }
    
    
    f32 Vector3::SqrMagnitude() const { return x * x + y * y + z * z; }

    Vector3 Vector3::Normalized() const
    {
        f32 mag = Magnitude();
        if (mag > 0)
            return *this / mag;
        return zero();
    }

    void Vector3::Normalize()
    {
        f32 mag = Magnitude();
        if (mag > 0)
        {
            x /= mag;
            y /= mag;
            z /= mag;
        }
    }

    
    
    Vector2 Vector3::ToVector2() const { return Vector2(x, y); }

    f32 Vector3::Dot(const Vector3 &a, const Vector3 &b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    Vector3 Vector3::Cross(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x);
    }

    f32 Vector3::Distance(const Vector3 &a, const Vector3 &b)
    {
        return (a - b).Magnitude();
    }

    Vector3 Vector3::Lerp(const Vector3 &a, const Vector3 &b, f32 t)
    {
        t = (t < 0) ? 0 : (t > 1) ? 1
                                  : t;
        return Vector3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
    }

    Vector3 Vector3::LerpUnclamped(const Vector3 &a, const Vector3 &b, f32 t)
    {
        return Vector3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
    }

    Vector3 operator*(f32 scalar, const Vector3 &v)
    {
        return Vector3(v.x * scalar, v.y * scalar, v.z * scalar);
    }

} // namespace Engine
