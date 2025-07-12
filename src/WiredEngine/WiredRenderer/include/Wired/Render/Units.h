/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_UNITS_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_UNITS_H

#include <glm/glm.hpp>

#include <cmath>
#include <numbers>

namespace Wired::Render
{
    struct Degrees
    {
        explicit Degrees(float _value)
            : value(std::fmod(_value, 360.0f))
        { }

        float value;
    };

    struct Radians
    {
        explicit Radians(float _value)
            : value(std::fmod(_value, 2.0f * std::numbers::pi_v<float>))
        { }

        float value;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_UNITS_H
