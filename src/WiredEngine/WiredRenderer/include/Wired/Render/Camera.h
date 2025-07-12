/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_CAMERA_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/constants.hpp>

namespace Wired::Render
{
    struct Camera
    {
        glm::vec3 position{0,0,0};
        glm::vec3 lookUnit{0,0,-1};
        glm::vec3 upUnit{0,1,0};
        glm::vec3 rightUnit{1,0,0};

        // Only used for 2D renders
        float scale{1.0f};

        // Only used for 3D renders
        float fovYDegrees{45.0f};
        float aspectRatio{1.0f};

        bool operator==(const Camera& other) const
        {
            if (!glm::all(glm::epsilonEqual(position, other.position, glm::epsilon<float>()))) { return false; }
            if (!glm::all(glm::epsilonEqual(lookUnit, other.lookUnit, glm::epsilon<float>()))) { return false; }
            if (!glm::all(glm::epsilonEqual(upUnit, other.upUnit, glm::epsilon<float>()))) { return false; }
            if (!glm::all(glm::epsilonEqual(rightUnit, other.rightUnit, glm::epsilon<float>()))) { return false; }
            if (!glm::epsilonEqual(scale, other.scale, glm::epsilon<float>())) { return false; }
            if (!glm::epsilonEqual(fovYDegrees, other.fovYDegrees, glm::epsilon<float>())) { return false; }
            if (!glm::epsilonEqual(aspectRatio, other.aspectRatio, glm::epsilon<float>())) { return false; }
            return true;
        }
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_CAMERA_H
