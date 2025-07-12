/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_RECT_H
#define WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_RECT_H

#include "Size2D.h"
#include "Point2D.h"

#include "../Compare.h"

#include <cstdint>

namespace NCommon
{
    /**
     * Helper class which defines a Rect located in 2D space.
     *
     * @tparam P Data type to use for the Rect's position
     * @tparam S Data type to use for the Rect's size
     */
    template <typename P, typename S>
    requires (std::is_floating_point_v<S> || std::is_unsigned_v<S>)
    struct Rect
    {
        using PosType = P;
        using SizeType = S;

        Rect() = default;

        Rect(const P& _x, const P& _y, const S& width, const S& height)
            : x(_x) , y(_y) , w(width) , h(height)
        { }

        Rect(const S& width, const S& height) : Rect(0, 0, width, height) {}
        explicit Rect(const std::pair<S, S>& p) : Rect(0, 0, p.first, p.second) {}
        explicit Rect(const Size2D<S>& s) : Rect(0, 0, s.w, s.h) {}
        Rect(const Point2DReal& point, const Size2D<S>& s) : Rect(point.x, point.y, s.w, s.h) {}

        [[nodiscard]] bool operator==(const Rect<P,S>& rhs) const
        {
            return AreEqual(x, rhs.x) && AreEqual(y, rhs.y) && AreEqual(w, rhs.w) && AreEqual(h, rhs.h);
        }

        [[nodiscard]] Size2D<S> GetSize() const noexcept
        {
            return Size2D<S>(w, h);
        }

        [[nodiscard]] const P& GetX() const { return x; }
        [[nodiscard]] const P& GetY() const { return y; }
        [[nodiscard]] const S& GetWidth() const { return w; }
        [[nodiscard]] const S& GetHeight() const { return h; }

        P x{}; P y{};
        S w{}; S h{};
    };

    using RectUInt  = Rect<unsigned int, unsigned int>;
    using RectReal  = Rect<float, float>;
}

#endif //WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_RECT_H
