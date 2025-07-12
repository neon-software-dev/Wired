/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_POINT3D_H
#define WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_POINT3D_H

#include <type_traits>

namespace NCommon
{
    template <typename T>
    requires (std::is_floating_point_v<T> || std::is_unsigned_v<T>)
    struct Point3D
    {
        Point3D(const T& _x, const T& _y, const T& _z) : x(_x), y(_y), z(_z) { }

        [[nodiscard]] const T& GetX() const noexcept { return x; }
        [[nodiscard]] const T& GetY() const noexcept { return y; }
        [[nodiscard]] const T& GetZ() const noexcept { return z; }

        T x;
        T y;
        T z;
    };

    using Point3DUInt = Point3D<unsigned int>;
    using Point3DReal = Point3D<float>;
}

#endif //WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_POINT3D_H
