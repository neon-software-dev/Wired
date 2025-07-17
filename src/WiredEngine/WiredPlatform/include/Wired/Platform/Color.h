/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_COLOR_H
#define WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_COLOR_H

#include <cstdint>

namespace Wired::Platform
{
    struct Color
    {
        Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a)
            : r(_r), g(_g), b(_b), a(_a)
        { }

        bool operator==(const Color& o) const
        {
            return r == o.r && g == o.g && b == o.b && a == o.a;
        }

        static Color Transparent() { return {0, 0, 0, 0}; }
        static Color White() { return {255, 255, 255, 255}; }
        static Color Black() { return {0, 0, 0, 255}; }
        static Color Red() { return {255, 0, 0, 255}; }
        static Color Green() { return {0, 255, 0, 255}; }
        static Color Blue() { return {0, 0, 255, 255}; }

        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
}

#endif //WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_COLOR_H
