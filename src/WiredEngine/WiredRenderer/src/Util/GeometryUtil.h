/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_UTIL_GEOMETRYUTIL_H
#define WIREDENGINE_WIREDRENDERER_SRC_UTIL_GEOMETRYUTIL_H

#include "Projection.h"
#include "Sphere.h"
#include "Plane.h"
#include "Ray.h"

#include <Wired/Render/AABB.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <optional>

namespace Wired::Render
{
    //
    // Hodgepodge of various geometry utility functions
    //

    /**
     * Calculates the clip code for the result of applying a projection to a point. (The clip code
     * sets a bit for each plane where the transformed point lies outside the area of the projection,
     * with a clip code of zero meaning the transformed point is within the projection).
     */
    [[nodiscard]] uint8_t CalculateClipCode(const glm::mat4& projection, const glm::vec3& point);

    /**
     * Returns whether a volume is trivially outside the bounds of a projectionFrustum.
     *
     * Note that this should only be used for imperfect culling logic; it will only say that the AABB
     * is outside of the projectionFrustum if it is *TRIVIALLY* outside of the projectionFrustum. More complex cases where
     * the AABB is outside multiple planes of the projectionFrustum, will err on the side of caution and will be
     * reported as not trivially outside the projectionFrustum, even if in actuality the AABB might not be visible
     * within the projectionFrustum.
     *
     * @param aabb The AABB to be transformed
     * @param projection The projectionFrustum transformation to apply
     *
     * @return Whether the AABB is trivially outside of the projectionFrustum.
     */
    [[nodiscard]] bool VolumeTriviallyOutsideProjection(const Volume& volume, const glm::mat4& projection);

    /**
     * Returns the point on the surface of the volume which is closest to the provided point.
     *
     * Warning! If the provided point is within the volume's bounds, the point itself will be returned.
     */
    [[nodiscard]] glm::vec3 SlidePointToVolume(const glm::vec3& point, const Volume& volume) noexcept;

    /**
     * @return The minimum distance between the provided point and volume (0.0f if the point is within
     * the volume already).
     */
    [[nodiscard]] float DistanceToVolume(const glm::vec3& point, const Volume& volume) noexcept;

    /**
     * @return Whether two volumes intersect
     */
    [[nodiscard]] bool Intersects(const Volume& a, const Volume& b) noexcept;

    /**
     * @return Whether a point intersects a volume
     */
    [[nodiscard]] bool Intersects(const glm::vec3& point, const Volume& volume) noexcept;

    /**
     * @return Whether a sphere and volume intersect
     */
    [[nodiscard]] bool Intersects(const Sphere& sphere, const Volume& volume) noexcept;

    /**
     * Calculates the distance along a ray to a plane. Returns a negative number if backwards
     * intersection. Returns std::nullopt if the ray and plane are (sufficiently) parallel
     * and do not intersect.
     *
     * Warning: This will return the distance along the ray to the plane, which is different
     * than the shortest distance from the ray origin to the plane.
     *
     * @return The distance along a ray to a plane or std::nullopt if no intersection
     */
    [[nodiscard]] std::optional<float> DistanceToPlane(const Ray& ray, const Plane& plane);

    /**
     * Determine the intersection point between a ray and a plane.
     *
     * @param ray The ray to test
     * @param plane The plane for the ray to intersect with
     * @param allowedBackwardsTravel Whether or not to allow a backwards intersection along the ray's direction
     *
     * @return The intersection point, or std::nullopt if the ray and plane are (sufficiently) parallel
     * and do not intersect, or std::nullopt if the plane is behind the ray and allowedBackwardsTravel is false.
     */
    [[nodiscard]] std::optional<glm::vec3> Intersection(const Ray& ray, const Plane& plane, bool allowedBackwardsTravel);

    /**
     * Applies the provided transform to the provided projection's (view-space) bounding points, and returns the
     * transformed bounding points.
     */
    [[nodiscard]] std::vector<glm::vec3> TransformedProjectionBounds(const Projection::Ptr& projection, const glm::mat4& transform);

    /**
     * Applies the provided transform to the provided projection's (view-space) bounding points, then returns an AABB
     * from the transformed points.
     */
    [[nodiscard]] AABB AABBForTransformedProjection(const Projection::Ptr& projection, const glm::mat4& transform);

    /**
     * @return The weighted center point of the provided points
     */
    [[nodiscard]] glm::vec3 GetCenterPoint(const std::vector<glm::vec3>& points);
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_UTIL_GEOMETRYUTIL_H
