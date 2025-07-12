/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_POINT2D_H
#define WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_POINT2D_H

namespace NCommon
{
    template <typename T>
    requires (std::is_floating_point_v<T> || std::is_unsigned_v<T>)
    struct Point2D
    {
        Point2D(const T& _x, const T& _y) : x(_x), y(_y) { }

        [[nodiscard]] const T& GetX() const noexcept { return x; }
        [[nodiscard]] const T& GetY() const noexcept { return y; }

        T x;
        T y;
    };

    using Point2DUInt = Point2D<unsigned int>;
    using Point2DReal = Point2D<float>;
}

#endif //WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_POINT2D_H
