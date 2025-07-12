/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_UTIL_RAY_H
#define WIREDENGINE_WIREDRENDERER_SRC_UTIL_RAY_H

#include <glm/glm.hpp>

#include <optional>

namespace Wired::Render
{
    struct Ray
    {
        Ray() = default;

        Ray(const glm::vec3& _originPoint, const glm::vec3& _dirUnit, const std::optional<float>& _length = std::nullopt)
            : originPoint(_originPoint)
            , dirUnit(_dirUnit)
            , length(_length)
        { }

        glm::vec3 originPoint{0}; // Origin point of the ray
        glm::vec3 dirUnit{0};     // Unit vector of the ray's direction
        std::optional<float> length;    // Optional length of the ray (unbounded if not set)
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_UTIL_RAY_H
