/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_TRIANGLE_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_TRIANGLE_H

#include <glm/glm.hpp>

namespace Wired::Render
{
    struct Triangle
    {
        Triangle(const glm::vec3& _p1, const glm::vec3& _p2, const glm::vec3& _p3)
            : p1(_p1)
            , p2(_p2)
            , p3(_p3)
        { }

        glm::vec3 p1;
        glm::vec3 p2;
        glm::vec3 p3;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_TRIANGLE_H
