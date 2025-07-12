/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_UTIL_SPHERE_H
#define WIREDENGINE_WIREDRENDERER_SRC_UTIL_SPHERE_H

#include <glm/glm.hpp>

namespace Wired::Render
{
    struct Sphere
    {
        Sphere() = default;

        Sphere(const glm::vec3& _center, float _radius)
            : center(_center)
            , radius(_radius)
        { }

        glm::vec3 center{0};    // The center point of the sphere
        float radius{0.0f};         // The radius of the sphere
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_UTIL_SPHERE_H