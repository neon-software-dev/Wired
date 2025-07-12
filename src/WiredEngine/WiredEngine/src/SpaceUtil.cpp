/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Engine/SpaceUtil.h>

namespace Wired::Engine
{

std::optional<NCommon::Point2DReal> ScreenSurfacePointToRenderSurfacePoint(const ScreenSurfacePoint& screenPoint,
                                                                           const NCommon::RectReal& screenBlitRect,
                                                                           const NCommon::RectReal& renderBlitRect)
{
    //
    // If the point isn't within the portion of the screen surface that received
    // the render, then bail out
    //
    if ((screenPoint.x < screenBlitRect.x) ||
        (screenPoint.x > (screenBlitRect.x + screenBlitRect.w)))
    {
        return std::nullopt;
    }

    if ((screenPoint.y < screenBlitRect.y) ||
        (screenPoint.y > (screenBlitRect.y + screenBlitRect.h)))
    {
        return std::nullopt;
    }

    const float renderXPercent = (screenPoint.x - screenBlitRect.x) / screenBlitRect.w;
    const float renderYPercent = (screenPoint.y - screenBlitRect.y) / screenBlitRect.h;

    const NCommon::Point2D renderSurfacePoint(
        renderBlitRect.x + (renderXPercent * renderBlitRect.w),
        renderBlitRect.y + (renderYPercent * renderBlitRect.h)
    );

    return renderSurfacePoint;
}

}
