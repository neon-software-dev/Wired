/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_UTIL_ORTHOPROJECTION_H
#define WIREDENGINE_WIREDRENDERER_SRC_UTIL_ORTHOPROJECTION_H

#include "Projection.h"

#include <expected>

namespace Wired::Render
{
    class OrthoProjection : public Projection
    {
        public:

            using Ptr = std::shared_ptr<OrthoProjection>;

        private:

            struct Tag{};

        public:

            static std::expected<Projection::Ptr, bool> From(const glm::vec3& nearMin,
                                                             const glm::vec3& nearMax,
                                                             const glm::vec3& farMin,
                                                             const glm::vec3& farMax);

            static std::expected<Projection::Ptr, bool> From(const float& width,
                                                             const float& height,
                                                             const float& nearDistance,
                                                             const float& farDistance);

            OrthoProjection(Tag,
                            const glm::vec3& nearMin,
                            const glm::vec3& nearMax,
                            const glm::vec3& farMin,
                            const glm::vec3& farMax);

            [[nodiscard]] bool operator==(const OrthoProjection& other) const;

            //
            // Projection
            //
            [[nodiscard]] Projection::Ptr Clone() const override;
            [[nodiscard]] bool Equals(const Projection* other) const override;
            [[nodiscard]] glm::mat4 GetProjectionMatrix() const noexcept override;
            [[nodiscard]] float GetNearPlaneDistance() const noexcept override;
            [[nodiscard]] float GetFarPlaneDistance() const noexcept override;
            [[nodiscard]] AABB GetAABB() const noexcept override;
            [[nodiscard]] std::vector<glm::vec3> GetBoundingPoints() const override;
            [[nodiscard]] glm::vec3 GetNearPlaneMin() const noexcept override;
            [[nodiscard]] glm::vec3 GetNearPlaneMax() const noexcept override;
            [[nodiscard]] glm::vec3 GetFarPlaneMin() const noexcept override;
            [[nodiscard]] glm::vec3 GetFarPlaneMax() const noexcept override;
            [[nodiscard]] bool SetNearPlaneDistance(const float& distance) override;
            [[nodiscard]] bool SetFarPlaneDistance(const float& distance) override;

        private:

            void ComputeAncillary();

        private:

            // Warning! If adding any members, make sure to update equality operator

            //
            // Coordinates of bottom-left and top-right points in the near and far planes.
            // Note that the points are in view-space and z values are always negative.
            //
            glm::vec3 m_nearMin{0.0f};
            glm::vec3 m_nearMax{0.0f};
            glm::vec3 m_farMin{0.0f};
            glm::vec3 m_farMax{0.0f};

            // Ancillary
            glm::mat4 m_projection{1};
            AABB m_aabb;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_UTIL_ORTHOPROJECTION_H
