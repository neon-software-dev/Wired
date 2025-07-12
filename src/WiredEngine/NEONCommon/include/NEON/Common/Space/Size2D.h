/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_SIZE2D_H
#define WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_SIZE2D_H

#include "../Compare.h"

namespace NCommon
{
    template <typename T>
    requires (std::is_floating_point_v<T> || std::is_unsigned_v<T>)
    struct Size2D
    {
        using Type = T;

        Size2D() = default;

        Size2D(const T& width, const T& height)
            : w(width), h(height)
        { }

        explicit Size2D(const std::pair<T, T>& _value) : Size2D(_value.first, _value.second) {}

        template <typename OtherT>
        [[nodiscard]] static Size2D<T> CastFrom(const Size2D<OtherT>& other)
        {
            return Size2D<T>(static_cast<Type>(other.w), static_cast<Type>(other.h));
        }

        [[nodiscard]] bool operator==(const Size2D<T>& rhs) const
        {
            return AreEqual(w, rhs.w) && AreEqual(h, rhs.h);
        }

        [[nodiscard]] const T& GetWidth() const { return w; }
        [[nodiscard]] const T& GetHeight() const { return h; }

        void SetWidth(T width) { w = width; }
        void SetHeight(T height) { h = height; }

        T w{};
        T h{};
    };

    using Size2DUInt = Size2D<unsigned int>;
    using Size2DReal = Size2D<float>;
}

#endif //WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_SIZE2D_H
