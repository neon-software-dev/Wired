/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_UTIL_FRUSTUMPROJECTION_H
#define WIREDENGINE_WIREDRENDERER_SRC_UTIL_FRUSTUMPROJECTION_H

#include "Projection.h"

#include <Wired/Render/Camera.h>

#include <glm/glm.hpp>

#include <expected>

namespace Wired::Render
{
    /**
     * Represents a *view-space* frustum defined by a near and far plane.
     *
     * All vectors passed in or returned are in the normal right-handed coordinate system
     * with positive z pointing backwards.
     *
     * All functions with near/far distance parameters or return values are always positive values.
     */
    class FrustumProjection : public Projection
    {
        public:

            using Ptr = std::shared_ptr<FrustumProjection>;

        private:

            struct Tag{};

        public:

            /**
             * Construct a view-space frustum from a render camera's parameters
             *
             * @param renderCamera The render camera to build from
             * @param nearDistance *POSITIVE* distance to the near plane (e.g. 0.1f)
             * @param farDistance *POSITIVE* distance to the far plane (e.g. 1000.0f)
             */
            [[nodiscard]] static std::expected<Projection::Ptr, bool> From(const Camera& camera, float nearDistance, float farDistance);

            /**
             * Construct a view-space frustum from a custom FOV and aspect ratio
             *
             * @param fovYDegrees Total (top to bottom) vertical FOV to be used
             * @param aspectRatio Aspect ratio of width to height for the frustum's near/far planes
             * @param nearDistance *POSITIVE* distance to the near plane (e.g. 0.1f)
             * @param farDistance *POSITIVE* distance to the far plane (e.g. 1000.0f)
             */
            [[nodiscard]] static std::expected<Projection::Ptr, bool> From(float fovYDegrees, float aspectRatio, float nearDistance, float farDistance);

            /**
             * Construct a view-space frustum from the min/max points on the far plane, and a distance to the
             * near plane.
             *
             * @param farMin Min point on the far plane
             * @param farMax Max point on the far plane
             * @param near *POSITIVE* distanceDistance to the near plane (e.g. 0.1f)
             */
            [[nodiscard]] static std::expected<Projection::Ptr, bool> From(const glm::vec3& farMin, const glm::vec3& farMax, float nearDistance);

            /**
             * Construct a view-space frustum from planes specified in tangents of half angles from the
             * center view axis.
             *
             * @param leftTanHalfAngle tan(fovX / 2.0f)
             * @param rightTanHalfAngle tan(fovX / 2.0f)
             * @param topTanHalfAngle tan(fovY / 2.0f)
             * @param bottomTanHalfAngle tan(fovY / 2.0f)
             * @param nearDistance *POSITIVE* distance to the near plane (e.g. 0.1f)
             * @param farDistance *POSITIVE* distance to the far plane (e.g. 1000.0f)
             */
            [[nodiscard]] static std::expected<Projection::Ptr, bool> FromTanHalfAngles(float leftTanHalfAngle,
                                                                                        float rightTanHalfAngle,
                                                                                        float topTanHalfAngle,
                                                                                        float bottomTanHalfAngle,
                                                                                        float nearDistance,
                                                                                        float farDistance);

            FrustumProjection(Tag, const glm::vec3& nearMin, const glm::vec3& nearMax, const glm::vec3& farMin, const glm::vec3& farMax);

            [[nodiscard]] bool operator==(const FrustumProjection& other) const;

            //
            // Projection
            //
            [[nodiscard]] Projection::Ptr Clone() const override;
            [[nodiscard]] bool Equals(const Projection* other) const override;
            [[nodiscard]] glm::mat4 GetProjectionMatrix() const noexcept override;
            [[nodiscard]] float GetNearPlaneDistance() const noexcept override;
            [[nodiscard]] float GetFarPlaneDistance() const noexcept override;
            [[nodiscard]] AABB GetAABB() const noexcept override;
            [[nodiscard]] std::vector<glm::vec3> GetBoundingPoints() const noexcept override;
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

            //
            // Tangents of (half) fov angles
            //
            float m_leftTanHalfAngle{0.0f};
            float m_rightTanHalfAngle{0.0f};
            float m_topTanHalfAngle{0.0f};
            float m_bottomTanHalfAngle{0.0f};

            //
            // Ancillary
            //
            glm::mat4 m_projection{1};
            AABB m_aabb;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_UTIL_FRUSTUMPROJECTION_H
