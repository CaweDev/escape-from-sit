/**
 * @file Color.cpp
 * @brief Implementation of Color struct
 */
#include "Color.hpp"

namespace Engine {



Color Color::FromRGBA(int r, int g, int b, int a) {
    return Color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}



Color Color::white() { return Color(1, 1, 1, 1); }


Color Color::black() { return Color(0, 0, 0, 1); }


Color Color::red() { return Color(1, 0, 0, 1); }


Color Color::green() { return Color(0, 1, 0, 1); }


Color Color::blue() { return Color(0, 0, 1, 1); }


Color Color::yellow() { return Color(1, 1, 0, 1); }


Color Color::cyan() { return Color(0, 1, 1, 1); }


Color Color::magenta() { return Color(1, 0, 1, 1); }


Color Color::gray() { return Color(0.5f, 0.5f, 0.5f, 1); }


Color Color::clear() { return Color(0, 0, 0, 0); }



Color Color::Lerp(const Color& a, const Color& b, f32 t) {
    t = (t < 0) ? 0 : (t > 1) ? 1 : t;
    return Color(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    );
}

} // namespace Engine
