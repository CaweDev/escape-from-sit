/*!
 * \file Mathf.hpp
 * \brief Static math utility class with Unity-like API
 */
#pragma once

#include "AEEngine.h"
#include <cmath>
#include <algorithm>

namespace Engine {

/*!
 * \class Mathf
 * \brief Static class containing common math functions
 */
class Mathf {
public:
    static constexpr f32 Deg2Rad = PI / 180.0f;
    static constexpr f32 Rad2Deg = 180.0f / PI;
    static constexpr f32 Epsilon = 0.00001f;
    static constexpr f32 Infinity = 1e30f;

    // Trigonometry (using Alpha Engine)
    static f32 Sin(f32 x) { return AESin(x); }
    static f32 Cos(f32 x) { return AECos(x); }
    static f32 Tan(f32 x) { return AETan(x); }
    static f32 Asin(f32 x) { return AEASin(x); }
    static f32 Acos(f32 x) { return AEACos(x); }
    static f32 Atan(f32 x) { return AEATan(x); }
    static f32 Atan2(f32 y, f32 x) { return std::atan2(y, x); }

    // Basic math
    static f32 Abs(f32 x) { return (x < 0) ? -x : x; }
    static int Abs(int x) { return (x < 0) ? -x : x; }
    static f32 Sqrt(f32 x) { return std::sqrt(x); }
    static f32 Pow(f32 x, f32 p) { return std::pow(x, p); }
    static f32 Exp(f32 x) { return std::exp(x); }
    static f32 Log(f32 x) { return std::log(x); }
    static f32 Log10(f32 x) { return std::log10(x); }
    
    // Rounding
    static f32 Floor(f32 x) { return std::floor(x); }
    static f32 Ceil(f32 x) { return std::ceil(x); }
    static f32 Round(f32 x) { return std::round(x); }
    static int FloorToInt(f32 x) { return static_cast<int>(std::floor(x)); }
    static int CeilToInt(f32 x) { return static_cast<int>(std::ceil(x)); }
    static int RoundToInt(f32 x) { return static_cast<int>(std::round(x)); }
    
    // Clamping and interpolation
    static f32 Clamp(f32 value, f32 min, f32 max) {
        return AEClamp(value, min, max);
    }
    
    static f32 Clamp01(f32 value) {
        return Clamp(value, 0.0f, 1.0f);
    }
    
    static int Clamp(int value, int min, int max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }
    
    static f32 Lerp(f32 a, f32 b, f32 t) {
        return a + (b - a) * Clamp01(t);
    }
    
    static f32 LerpUnclamped(f32 a, f32 b, f32 t) {
        return a + (b - a) * t;
    }
    
    static f32 InverseLerp(f32 a, f32 b, f32 value) {
        if (Abs(b - a) < Epsilon) return 0;
        return Clamp01((value - a) / (b - a));
    }
    
    // Comparison
    static f32 Min(f32 a, f32 b) { return AEMin(a, b); }
    static f32 Max(f32 a, f32 b) { return AEMax(a, b); }
    static int Min(int a, int b) { return (a < b) ? a : b; }
    static int Max(int a, int b) { return (a > b) ? a : b; }
    
    // Sign
    static f32 Sign(f32 x) {
        if (x > 0) return 1.0f;
        if (x < 0) return -1.0f;
        return 0.0f;
    }
    
    // Movement helpers
    static f32 MoveTowards(f32 current, f32 target, f32 maxDelta) {
        if (Abs(target - current) <= maxDelta) return target;
        return current + Sign(target - current) * maxDelta;
    }
    
    static f32 SmoothStep(f32 from, f32 to, f32 t) {
        t = Clamp01(t);
        t = t * t * (3.0f - 2.0f * t);
        return from + (to - from) * t;
    }
    
    // Angle helpers
    static f32 DeltaAngle(f32 current, f32 target) {
        f32 delta = std::fmod(target - current, 360.0f);
        if (delta > 180.0f) delta -= 360.0f;
        if (delta < -180.0f) delta += 360.0f;
        return delta;
    }
    
    // Approximately equal (for f32ing point comparison)
    static bool Approximately(f32 a, f32 b) {
        return Abs(a - b) < Epsilon;
    }

private:
    Mathf() = delete; // Static class, no instantiation
};

} // namespace Engine
