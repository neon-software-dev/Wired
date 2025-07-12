/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_SIZE3D_H
#define WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_SIZE3D_H

#include "../Compare.h"

#include <utility>

namespace NCommon
{
    template <typename T>
    requires (std::is_floating_point_v<T> || std::is_unsigned_v<T>)
    struct Size3D
    {
        using Type = T;

        Size3D() = default;

        Size3D(const T& width, const T& height, const T& depth)
            : w(width), h(height), d(depth)
        { }

        explicit Size3D(const std::tuple<T, T, T>& _value) : Size3D(std::get<0>(_value), std::get<1>(_value), std::get<2>(_value)) {}

        template <typename OtherT>
        [[nodiscard]] static Size3D<T> CastFrom(const Size3D<OtherT>& other)
        {
            return Size3D<T>(static_cast<Type>(other.w), static_cast<Type>(other.h), static_cast<Type>(other.d));
        }

        [[nodiscard]] bool operator==(const Size3D<T>& rhs) const
        {
            return AreEqual(w, rhs.w) && AreEqual(h, rhs.h) && AreEqual(d, rhs.d);
        }

        [[nodiscard]] const T& GetWidth() const { return w; }
        [[nodiscard]] const T& GetHeight() const { return h; }
        [[nodiscard]] const T& GetDepth() const { return d; }

        void SetWidth(T width) { w = width; }
        void SetHeight(T height) { h = height; }
        void SetDepth(T depth) { d = depth; }

        T w{};
        T h{};
        T d{};
    };

    using Size3DUInt = Size3D<unsigned int>;
    using Size3DReal = Size3D<float>;
}

#endif //WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_SIZE3D_H
