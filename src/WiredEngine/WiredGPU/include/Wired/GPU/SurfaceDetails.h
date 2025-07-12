/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_SURFACEDETAILS_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_SURFACEDETAILS_H

#include <NEON/Common/Space/Size2D.h>

#include <optional>

namespace Wired::GPU
{
    struct SurfaceDetails
    {
        virtual ~SurfaceDetails() = default;

        NCommon::Size2DUInt pixelSize{0, 0};
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_SURFACEDETAILS_H
