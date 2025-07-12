/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_ROTATION_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_ROTATION_H

#include "Units.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <utility>
#include <optional>
#include <cmath>

namespace Wired::Render
{
    /**
     * Defines a rotation operation, around an optional point
     */
    struct Rotation
    {
        Rotation(const Degrees& degrees, const glm::vec3& rotAxis, const std::optional<glm::vec3>& _rotPoint = std::nullopt);
        Rotation(const Radians& radians, const glm::vec3& rotAxis, const std::optional<glm::vec3>& _rotPoint = std::nullopt);
        explicit Rotation(const glm::quat& _rotation, const std::optional<glm::vec3>& _rotPoint = std::nullopt);

        [[nodiscard]] glm::quat ApplyToOrientation(const glm::quat& in) const;
        [[nodiscard]] glm::vec3 ApplyToPosition(const glm::vec3& in) const;

        glm::quat rotation{glm::identity<glm::quat>()};
        std::optional<glm::vec3> rotPoint;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_ROTATION_H
