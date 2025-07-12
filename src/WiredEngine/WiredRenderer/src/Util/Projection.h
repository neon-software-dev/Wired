/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_UTIL_PROJECTION_H
#define WIREDENGINE_WIREDRENDERER_SRC_UTIL_PROJECTION_H

#include <Wired/Render/AABB.h>

#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace Wired::Render
{
    class Projection
    {
        public:

            using Ptr = std::shared_ptr<Projection>;

        public:

            virtual ~Projection() = default;

            [[nodiscard]] virtual Ptr Clone() const = 0;
            [[nodiscard]] virtual bool Equals(const Projection* other) const = 0;

            [[nodiscard]] virtual glm::mat4 GetProjectionMatrix() const noexcept = 0;

            [[nodiscard]] virtual float GetNearPlaneDistance() const noexcept = 0;
            [[nodiscard]] virtual float GetFarPlaneDistance() const noexcept = 0;
            [[nodiscard]] virtual AABB GetAABB() const noexcept = 0;
            [[nodiscard]] virtual std::vector<glm::vec3> GetBoundingPoints() const = 0;
            [[nodiscard]] virtual glm::vec3 GetNearPlaneMin() const noexcept = 0;
            [[nodiscard]] virtual glm::vec3 GetNearPlaneMax() const noexcept = 0;
            [[nodiscard]] virtual glm::vec3 GetFarPlaneMin() const noexcept = 0;
            [[nodiscard]] virtual glm::vec3 GetFarPlaneMax() const noexcept = 0;

            [[nodiscard]] virtual bool SetNearPlaneDistance(const float& distance) = 0;
            [[nodiscard]] virtual bool SetFarPlaneDistance(const float& distance) = 0;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_UTIL_PROJECTION_H
