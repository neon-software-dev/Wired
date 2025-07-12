/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_SURFACE_H
#define WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_SURFACE_H

#include "Size2D.h"

namespace NCommon
{
    /**
     * A surface is a 2D area with a specific size
     */
    struct Surface
    {
        explicit Surface(const Size2DUInt& _size) : size(_size) { }
        explicit Surface(const Size2DUInt::Type& w, const Size2DUInt::Type& h) : size(w, h) { }

        [[nodiscard]] const Size2DUInt& GetSize() const noexcept { return size; }

        Size2DUInt size;
    };
}

#endif //WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_SURFACE_H
