/**
 * @file Color.cpp
 * @brief Implementation of Color struct
 */
#include "Color.hpp"

namespace Engine {

/*!
 * \brief [AUTO] FromRGBA
 * \param r [in]
 * \param g [in]
 * \param b [in]
 * \param a [in]
 * \return 
 */
Color Color::FromRGBA(int r, int g, int b, int a) {
    return Color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

/*!
 * \brief [AUTO] white
 * \return 
 */
Color Color::white() { return Color(1, 1, 1, 1); }
/*!
 * \brief [AUTO] black
 * \return 
 */
Color Color::black() { return Color(0, 0, 0, 1); }
/*!
 * \brief [AUTO] red
 * \return 
 */
Color Color::red() { return Color(1, 0, 0, 1); }
/*!
 * \brief [AUTO] green
 * \return 
 */
Color Color::green() { return Color(0, 1, 0, 1); }
/*!
 * \brief [AUTO] blue
 * \return 
 */
Color Color::blue() { return Color(0, 0, 1, 1); }
/*!
 * \brief [AUTO] yellow
 * \return 
 */
Color Color::yellow() { return Color(1, 1, 0, 1); }
/*!
 * \brief [AUTO] cyan
 * \return 
 */
Color Color::cyan() { return Color(0, 1, 1, 1); }
/*!
 * \brief [AUTO] magenta
 * \return 
 */
Color Color::magenta() { return Color(1, 0, 1, 1); }
/*!
 * \brief [AUTO] gray
 * \return 
 */
Color Color::gray() { return Color(0.5f, 0.5f, 0.5f, 1); }
/*!
 * \brief [AUTO] clear
 * \return 
 */
Color Color::clear() { return Color(0, 0, 0, 0); }

/*!
 * \brief [AUTO] Lerp
 * \param a [in]
 * \param b [in]
 * \param t [in]
 * \return 
 */
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
