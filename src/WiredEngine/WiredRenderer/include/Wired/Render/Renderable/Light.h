/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERABLE_LIGHT_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERABLE_LIGHT_H

#include "../Id.h"

#include <glm/glm.hpp>

namespace Wired::Render
{
    // Warning - Can't change the order of these values without syncing shaders to the changed values
    enum class LightType
    {
        Point,
        Spotlight,
        Directional
    };

    // Warning - Can't change the order of these values without syncing shaders to the changed values
    enum class AttenuationMode
    {
        None,
        Linear,
        Exponential
    };

    struct Light
    {
        LightId id{};
        LightType type{};
        bool castsShadows{true};
        glm::vec3 worldPos;
        glm::vec3 color;
        AttenuationMode attenuation{};
        glm::vec3 directionUnit{0,0,-1};
        float areaOfEffect{360.0f};
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERABLE_LIGHT_H
