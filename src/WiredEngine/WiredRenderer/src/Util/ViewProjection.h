/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_UTIL_VIEWPROJECTION_H
#define WIREDENGINE_WIREDRENDERER_SRC_UTIL_VIEWPROJECTION_H

#include "Projection.h"
#include "GeometryUtil.h"

#include <Wired/Render/AABB.h>

#include <glm/glm.hpp>
#include <glm/ext/matrix_relational.hpp>

#include <utility>

namespace Wired::Render
{
    struct ViewProjection
    {
        ViewProjection() = default;

        ViewProjection(const glm::mat4& _viewTransform, const Projection::Ptr& _projectionTransform)
            : viewTransform(_viewTransform)
            , projectionTransform(_projectionTransform->Clone())
        { }

        ViewProjection(const ViewProjection& other)
            : viewTransform(other.viewTransform)
        {
            if (other.projectionTransform)
            {
                projectionTransform = other.projectionTransform->Clone();
            }
        }

        ViewProjection& operator=(const ViewProjection& other)
        {
            if (this == &other) return *this;

            viewTransform = other.viewTransform;
            projectionTransform = other.projectionTransform->Clone();

            return *this;
        }

        ViewProjection(ViewProjection&& other) noexcept
            : viewTransform(std::exchange(other.viewTransform, {}))
            , projectionTransform(std::exchange(other.projectionTransform, nullptr))
        {}

        ViewProjection& operator=(ViewProjection&& other) noexcept
        {
            std::swap(viewTransform, other.viewTransform);
            std::swap(projectionTransform, other.projectionTransform);
            return *this;
        }

        [[nodiscard]] bool operator==(const ViewProjection& other) const
        {
            if (!glm::all(glm::equal(viewTransform, other.viewTransform))) { return false; }

            if ((projectionTransform == nullptr) != (other.projectionTransform == nullptr)) { return false; }
            if (projectionTransform)
            {
                if (!projectionTransform->Equals(projectionTransform.get())) { return false; }
            }

            return true;
        }

        [[nodiscard]] glm::mat4 GetTransformation() const
        {
            return projectionTransform->GetProjectionMatrix() * viewTransform;
        }

        /**
         * Positions the projection's bounds in world space via the view transform and
         * computes an AABB from the world-spaced points. Returns a bounding box of what
         * portion of world-space the ViewProjection covers.
         */
        [[nodiscard]] AABB GetWorldSpaceAABB() const
        {
            return AABBForTransformedProjection(
                projectionTransform,
                glm::inverse(viewTransform)
            );
        }

        [[nodiscard]] std::vector<glm::vec3> GetWorldSpaceBoundingPoints() const
        {
            return TransformedProjectionBounds(
                projectionTransform,
                glm::inverse(viewTransform)
            );
        }

        // Warning! If adding any members, make sure to update equality operator

        glm::mat4 viewTransform{1};
        Projection::Ptr projectionTransform;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_UTIL_VIEWPROJECTION_H
