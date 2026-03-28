/**
 * @file Vector2.hpp
 */
#pragma once

#include "AEEngine.h"

namespace Engine
{
    struct Vector3;

    struct Vector2
    {
        f32 x, y;

        Vector2();
        Vector2(f32 x, f32 y);
        Vector2(const AEVec2 &v);
        Vector2(const Vector3 &v);

        static Vector2 zero();
        static Vector2 one();
        static Vector2 up();
        static Vector2 down();
        static Vector2 left();
        static Vector2 right();

        Vector2 operator+(const Vector2 &other) const;
        Vector2 operator-(const Vector2 &other) const;
        Vector2 operator*(f32 scalar) const;
        Vector2 operator/(f32 scalar) const;
        Vector2 operator-() const;

        Vector2 &operator+=(const Vector2 &other);
        Vector2 &operator-=(const Vector2 &other);
        Vector2 &operator*=(f32 scalar);
        Vector2 &operator/=(f32 scalar);

        bool operator==(const Vector2 &other) const;
        bool operator!=(const Vector2 &other) const;

        f32 Magnitude() const;
        f32 SqrMagnitude() const;

        Vector2 Normalized() const;
        void Normalize();

        static f32 Dot(const Vector2 &a, const Vector2 &b);
        static f32 Distance(const Vector2 &a, const Vector2 &b);
        static Vector2 Lerp(const Vector2 &a, const Vector2 &b, f32 t);
        static Vector2 LerpUnclamped(const Vector2 &a, const Vector2 &b, f32 t);

        AEVec2 ToAEVec2() const;
    };

    Vector2 operator*(f32 scalar, const Vector2 &v);

} // namespace Engine