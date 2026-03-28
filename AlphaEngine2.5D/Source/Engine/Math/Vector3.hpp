/**
 * @file Vector3.hpp
 */
#pragma once

#include "AEEngine.h"

namespace Engine
{
    struct Vector2;

    struct Vector3
    {
        f32 x, y, z;

        Vector3();
        Vector3(f32 x, f32 y, f32 z = 0);
        Vector3(const Vector2 &v, f32 z = 0);

        static Vector3 zero();
        static Vector3 one();
        static Vector3 up();
        static Vector3 down();
        static Vector3 left();
        static Vector3 right();
        static Vector3 forward();
        static Vector3 back();

        Vector3 operator+(const Vector3 &other) const;
        Vector3 operator-(const Vector3 &other) const;
        Vector3 operator*(f32 scalar) const;
        Vector3 operator/(f32 scalar) const;
        Vector3 operator-() const;

        Vector3 &operator+=(const Vector3 &other);
        Vector3 &operator-=(const Vector3 &other);
        Vector3 &operator*=(f32 scalar);
        Vector3 &operator/=(f32 scalar);

        bool operator==(const Vector3 &other) const;
        bool operator!=(const Vector3 &other) const;

        f32 Magnitude() const;
        f32 SqrMagnitude() const;

        Vector3 Normalized() const;
        void Normalize();

        Vector2 ToVector2() const;

        static f32 Dot(const Vector3 &a, const Vector3 &b);
        static Vector3 Cross(const Vector3 &a, const Vector3 &b);
        static f32 Distance(const Vector3 &a, const Vector3 &b);
        static Vector3 Lerp(const Vector3 &a, const Vector3 &b, f32 t);
        static Vector3 LerpUnclamped(const Vector3 &a, const Vector3 &b, f32 t);
    };

    Vector3 operator*(f32 scalar, const Vector3 &v);

} // namespace Engine