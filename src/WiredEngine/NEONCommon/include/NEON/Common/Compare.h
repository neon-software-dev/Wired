/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_COMPARE_H
#define WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_COMPARE_H

#include <type_traits>
#include <limits>
#include <cmath>

namespace NCommon
{
    template <typename T>
    requires (std::is_integral_v<T>)
    [[nodiscard]] bool AreEqualT(const T& lhs, const T& rhs)
    {
        return lhs == rhs;
    }

    template <typename T>
    requires (std::is_floating_point_v<T>)
    [[nodiscard]] bool AreEqualT(const T& lhs, const T& rhs)
    {
        return std::fabs(lhs - rhs) < std::numeric_limits<T>::epsilon();
    }

    template <typename T>
    [[nodiscard]] bool AreEqual(const T& lhs, const T& rhs)
    {
        return AreEqualT(lhs, rhs);
    }
}

#endif //WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_COMPARE_H
