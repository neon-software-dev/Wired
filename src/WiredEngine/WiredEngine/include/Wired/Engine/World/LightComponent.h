/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_LIGHTCOMPONENT_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_LIGHTCOMPONENT_H

#include <Wired/Render/Renderable/Light.h>

#include <glm/glm.hpp>

namespace Wired::Engine
{
    struct LightComponent
    {
        Render::LightType type{Render::LightType::Point};
        bool castsShadows{false};
        glm::vec3 color{1};
        Render::AttenuationMode attenuationMode{Render::AttenuationMode::Linear};

        /**
         * The world-space unit vector which describes the direction the light is pointed.
         * For an omni-directional light, the value doesn't matter.
         */
        glm::vec3 directionUnit{0,0,-1};

        /**
         * Value to specify in which way the emitted light is restricted. Means something
         * different for each light type.
         *
         * [Point Lights]
         * Represents the degree width of the cone of light that the light emits, pointing in
         * the light's direction. Should be set to 360.0 for an omni-directional light, and valid
         * values are [0.0..360.0].
         *
         * [Spot Lights]
         * Represents the degree width of the cone of light that the light emits, pointing in
         * the light's direction. Should ideally be set to 90.0 or lower for decent shadow quality,
         * and valid values are [0.0..180.0].
         *
         * [Directional Lights]
         * Represents the world-space light plane disk radius of the emitted light. Should be set
         * to the special case value of 0.0f to represent no limitation of area of effect.
         * Any non-(sufficiently)zero value represents a disk radius from which to emit light from
         * the light's plane.
         */
        float areaOfEffect{360.0f};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_LIGHTCOMPONENT_H
