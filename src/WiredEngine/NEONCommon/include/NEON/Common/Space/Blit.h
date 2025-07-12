/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_BLIT_H
#define WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_BLIT_H

#include "Rect.h"

#include "../Build.h"
#include "../SharedLib.h"

#include <utility>

namespace NCommon
{
    enum class BlitType
    {
        /**
        * The source will be scaled up/down as needed so the target is full of content.
        * Crops the source if the source and target aspect ratios don't match.
        */
        CenterCrop,

        /**
         * The source will be scaled up/down as needed to fully fit within the target.
         * Introduces un-drawn area (black bars) if the source and target aspect ratios
         * don't match.
         */
        CenterInside
    };

    /**
     * Calculates blit rects for blitting from a source image onto a target image.
     *
     * Returns two rects, respectively:
     * 1) A source blit rect representing the blit selection from the source image.
     * 2) A dest blit rect representing the blit selection onto the dest image.
     */
     [[nodiscard]] NEON_PUBLIC std::pair<RectReal, RectReal> CalculateBlitRects(
        const BlitType& blitType,
        const Size2DReal& sourceSize,
        const Size2DReal& targetSize);
}

#endif //WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_SPACE_BLIT_H
