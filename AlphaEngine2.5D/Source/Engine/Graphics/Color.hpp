/**
 * @file Color.hpp
 * @brief RGBA Color struct with presets
 */
#pragma once

#include "AEEngine.h"

namespace Engine {

/**
 * @struct Color
 * @brief RGBA color with values 0-1
 */
struct Color {
    f32 r, g, b, a;

    // Constructors
    Color() : r(1), g(1), b(1), a(1) {}
    Color(f32 r, f32 g, f32 b, f32 a = 1.0f) : r(r), g(g), b(b), a(a) {}
    
    // From 0-255 values
    static Color FromRGBA(int r, int g, int b, int a = 255);

    // Preset colors
    static Color white();
    static Color black();
    static Color red();
    static Color green();
    static Color blue();
    static Color yellow();
    static Color cyan();
    static Color magenta();
    static Color gray();
    static Color clear();

    // Operators
    Color operator*(f32 scalar) const { return Color(r * scalar, g * scalar, b * scalar, a); }
    Color operator*(const Color& other) const { return Color(r * other.r, g * other.g, b * other.b, a * other.a); }
    
    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    bool operator!=(const Color& other) const { return !(*this == other); }

    // Interpolation
    static Color Lerp(const Color& a, const Color& b, f32 t);

    /** Convert to AE u32 color (0xAARRGGBB) */
    u32 ToU32() const {
        u8 ar = static_cast<u8>(r * 255.0f);
        u8 ag = static_cast<u8>(g * 255.0f);
        u8 ab = static_cast<u8>(b * 255.0f);
        u8 aa = static_cast<u8>(a * 255.0f);
        return (aa << 24) | (ar << 16) | (ag << 8) | ab;
    }
};

} // namespace Engine
