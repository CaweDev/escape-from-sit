/**
 * @file Vector2.cpp
 */
#include "Vector2.hpp"
#include "Vector3.hpp"
#include <cmath>

namespace Engine
{
    Vector2::Vector2() : x(0), y(0) {}
    Vector2::Vector2(f32 x, f32 y) : x(x), y(y) {}
    Vector2::Vector2(const AEVec2 &v) : x(v.x), y(v.y) {}
    Vector2::Vector2(const Vector3 &v) : x(v.x), y(v.y) {}

    /*!
     * \brief [AUTO] zero
     * \return 
     */
    Vector2 Vector2::zero() { return Vector2(0, 0); }
    /*!
     * \brief [AUTO] one
     * \return 
     */
    Vector2 Vector2::one() { return Vector2(1, 1); }
    /*!
     * \brief [AUTO] up
     * \return 
     */
    Vector2 Vector2::up() { return Vector2(0, 1); }
    /*!
     * \brief [AUTO] down
     * \return 
     */
    Vector2 Vector2::down() { return Vector2(0, -1); }
    /*!
     * \brief [AUTO] left
     * \return 
     */
    Vector2 Vector2::left() { return Vector2(-1, 0); }
    /*!
     * \brief [AUTO] right
     * \return 
     */
    Vector2 Vector2::right() { return Vector2(1, 0); }

    Vector2 Vector2::operator+(const Vector2 &other) const { return Vector2(x + other.x, y + other.y); }
    Vector2 Vector2::operator-(const Vector2 &other) const { return Vector2(x - other.x, y - other.y); }
    Vector2 Vector2::operator*(f32 scalar) const { return Vector2(x * scalar, y * scalar); }
    Vector2 Vector2::operator/(f32 scalar) const { return Vector2(x / scalar, y / scalar); }
    Vector2 Vector2::operator-() const { return Vector2(-x, -y); }

    Vector2 &Vector2::operator+=(const Vector2 &other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2 &Vector2::operator-=(const Vector2 &other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vector2 &Vector2::operator*=(f32 scalar)
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    Vector2 &Vector2::operator/=(f32 scalar)
    {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    bool Vector2::operator==(const Vector2 &other) const { return x == other.x && y == other.y; }
    bool Vector2::operator!=(const Vector2 &other) const { return !(*this == other); }

    /*!
     * \brief [AUTO] Magnitude
     * \return 
     */
    f32 Vector2::Magnitude() const { return std::sqrt(x * x + y * y); }
    /*!
     * \brief [AUTO] SqrMagnitude
     * \return 
     */
    f32 Vector2::SqrMagnitude() const { return x * x + y * y; }

    Vector2 Vector2::Normalized() const
    {
        f32 mag = Magnitude();
        if (mag > 0)
            return *this / mag;
        return zero();
    }

    void Vector2::Normalize()
    {
        f32 mag = Magnitude();
        if (mag > 0)
        {
            x /= mag;
            y /= mag;
        }
    }

    f32 Vector2::Dot(const Vector2 &a, const Vector2 &b)
    {
        return a.x * b.x + a.y * b.y;
    }

    f32 Vector2::Distance(const Vector2 &a, const Vector2 &b)
    {
        return (a - b).Magnitude();
    }

    Vector2 Vector2::Lerp(const Vector2 &a, const Vector2 &b, f32 t)
    {
        t = (t < 0) ? 0 : (t > 1) ? 1
                                  : t;
        return Vector2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
    }

    Vector2 Vector2::LerpUnclamped(const Vector2 &a, const Vector2 &b, f32 t)
    {
        return Vector2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
    }

    AEVec2 Vector2::ToAEVec2() const
    {
        AEVec2 v;
        v.x = x;
        v.y = y;
        return v;
    }

    Vector2 operator*(f32 scalar, const Vector2 &v)
    {
        return Vector2(v.x * scalar, v.y * scalar);
    }

} // namespace Engine
