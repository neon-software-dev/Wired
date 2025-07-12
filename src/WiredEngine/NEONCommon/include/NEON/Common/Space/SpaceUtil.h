/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_SPACEUTIL_H
#define WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_SPACEUTIL_H

#include "Point2D.h"
#include "Point3D.h"
#include "Surface.h"
#include "Size2D.h"

namespace NCommon
{
    //
    // Helper functions for working with points/sizes/surfaces and converting
    // between them.
    //
    // WARNING! All functions in here require particular coordinate system conventions:
    // 1) All 2D points specified in a coordinate system with +X to the right, and +Y upwards.
    // 2) All 2D surfaces specified in a coordinate system with +X to the right, and +Y downwards.
    //

    /**
     * Maps a 2D point from point-space to surface-space, where surfaceOriginMapPoint
     * is a point in surface-space which is where the origin of the point-space
     * should be aligned with.
     */
    template <typename Point2DT>
    requires(
        std::is_base_of_v<Point2DReal, Point2DT>
    )
    [[nodiscard]] Point2DT Map2DPointToSurfaceSpace(const Point2DT& point, const Point2DT& surfaceOriginMapPoint)
    {
        return {
            point.x + surfaceOriginMapPoint.x,
            surfaceOriginMapPoint.y - point.y
        };
    }

    /**
     * Maps a 2D point from point-space to surface-space on a given surface, where the
     * origin of the point-space should align with the center of the surface.
     */
    template <typename SurfaceT, typename Point2DT>
    requires(
        std::is_base_of_v<Surface, SurfaceT> &&
        std::is_base_of_v<Point2DReal, Point2DT>
    )
    [[nodiscard]] Point2DT Map2DPointToSurfaceSpaceCenterOrigin(const Point2DT& point, const SurfaceT& surface)
    {
        return Map2DPointToSurfaceSpace(point, {(float)surface.size.w / 2.0f, (float)surface.size.h / 2.0f});
    }

    /**
     * Maps a 2D point from surface-space to a 2D point in point-space, where surfaceOriginMapPoint
     * is a point in surface-space which is where the origin of the point-space should be aligned with.
     */
    template <typename Point2DInT, typename Point2DOutT>
    requires(
        std::is_base_of_v<Point2DReal, Point2DInT> &&
        std::is_base_of_v<Point2DReal, Point2DOutT>
    )
    [[nodiscard]] Point2DOutT MapSurfacePointToPointSpace(const Point2DInT& point, const Point2DInT& surfaceOriginMapPoint)
    {
        return {
            point.x - surfaceOriginMapPoint.x,
            surfaceOriginMapPoint.y - point.y
        };
    }

    /**
     * Maps a 2D point from surface-space to a 3D point in point-space, where surfaceOriginMapPoint
     * is a point in surface-space which is where the origin of the point-space should be aligned with,
     * and where the 3D point's z-coordinate is defaulted to zero.
     */
    template <typename Point2DInT, typename Point3DOutT>
    requires(
        std::is_base_of_v<Point2DReal, Point2DInT> &&
        std::is_base_of_v<Point3DReal, Point3DOutT>
    )
    [[nodiscard]] Point3DOutT MapSurfacePointToPointSpace(const Point2DInT& point, const Point2DInT& surfaceOriginMapPoint)
    {
        return {
            point.x - surfaceOriginMapPoint.x,
            surfaceOriginMapPoint.y - point.y,
            0.0f
        };
    }

    /**
     * Maps a 2D point from surface-space to a 2D point in point-space, where the origin of the point-space
     * should align with the center of the surface.
     */
    template <typename SurfaceT, typename Point2DInT, typename Point2DOutT>
    requires(
        std::is_base_of_v<Surface, SurfaceT> &&
        std::is_base_of_v<Point2DReal, Point2DInT> &&
        std::is_base_of_v<Point2DReal, Point2DOutT>
    )
    [[nodiscard]] Point2DOutT MapSurfacePointToPointSpaceCenterOrigin(const Point2DInT& point, const SurfaceT& surface)
    {
        return MapSurfacePointToPointSpace<Point2DInT, Point2DOutT>(point, {(float)surface.size.w / 2.0f, (float)surface.size.h / 2.0f});
    }

    /**
     * Maps a 2D point from surface-space to a 3D point in point-space, where the origin of the point-space
     * should align with the center of the surface, and where the 3D point's z-coordinate is defaulted to zero.
     */
    template <typename SurfaceT, typename Point2DInT, typename Point3DOutT>
    requires(
        std::is_base_of_v<Surface, SurfaceT> &&
        std::is_base_of_v<Point2DReal, Point2DInT> &&
        std::is_base_of_v<Point3DReal, Point3DOutT>
    )
    [[nodiscard]] Point3DOutT MapSurfacePointToPointSpaceCenterOrigin(const Point2DInT& point, const SurfaceT& surface)
    {
        return MapSurfacePointToPointSpace<Point2DInT, Point3DOutT>(point, {(float)surface.size.w / 2.0f, (float)surface.size.h / 2.0f});
    }

    /**
     * Maps a size linearly between surfaces - e.g. if outSurface is twice the size of inSurface, the resulting
     * size will be twice the size of the input size.
     */
    template <typename SizeInT, typename SizeOutT>
    [[nodiscard]] auto MapSizeBetweenSurfaces(const SizeInT& size, const Surface& inSurface, const Surface& outSurface)
    {
        return SizeOutT{
            ((float)size.w * ((float)outSurface.size.w / (float)inSurface.size.w)),
            ((float)size.h * ((float)outSurface.size.h / (float)inSurface.size.h))
        };
    }

    /**
     * Maps a 2D point between surfaces. Effectively scales the point's position by the
     * ratio between the outSurface and inSurface's dimensions. E.g., a (10,10) point
     * in a 100x100 surface would be mapped to (20,20) on a 200x200 surface.
     */
    template <typename Point2DInT, typename Point2DOutT>
    requires(
        std::is_base_of_v<Point2DReal, Point2DInT> &&
        std::is_base_of_v<Point2DReal, Point2DOutT>
    )
    [[nodiscard]] Point2DOutT Map2DPointBetweenSurfaces(const Point2DInT& inPoint,
                                                        const Surface& inSurface,
                                                        const Surface& outSurface)
    {
        return Point2DOutT{
            ((float)inPoint.x * ((float)outSurface.size.w / (float)inSurface.size.w)),
            ((float)inPoint.y * ((float)outSurface.size.h / (float)inSurface.size.h))
        };
    }

    /**
     * Maps a 3D point between surfaces. Effectively scales the point's position by the
     * ratio between the outSurface and inSurface's dimensions, while keeping the point's
     * original z value. E.g., a (10,10,10) point in a 100x100 surface would be mapped to
     * (20,20,10) on a 200x200 surface.
     */
    template <typename Point3DInT, typename Point3DOutT>
    requires(
        std::is_base_of_v<Point3DReal, Point3DInT> &&
        std::is_base_of_v<Point3DReal, Point3DOutT>
    )
    [[nodiscard]] Point3DOutT Map3DPointBetweenSurfaces(const Point3DInT& inPoint,
                                                        const Surface& inSurface,
                                                        const Surface& outSurface)
    {
        return Point3DOutT{
            ((float)inPoint.x * ((float)outSurface.size.w / (float)inSurface.size.w)),
            ((float)inPoint.y * ((float)outSurface.size.h / (float)inSurface.size.h)),
            inPoint.z
        };
    }
}

#endif //WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_SPACEUTIL_H
