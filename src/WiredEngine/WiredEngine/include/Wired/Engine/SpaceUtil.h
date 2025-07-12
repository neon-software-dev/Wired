/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_SPACEUTIL_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_SPACEUTIL_H

#include <Wired/Engine/EngineCommon.h>

#include <NEON/Common/SharedLib.h>
#include <NEON/Common/Build.h>
#include <NEON/Common/Space/Size2D.h>
#include <NEON/Common/Space/Rect.h>
#include <NEON/Common/Space/Surface.h>

#include <optional>

namespace Wired::Engine
{
    /**
     * Converts a screen surface point to a render surface point. Mostly used when the user
     * clicks on the window to map that click back to a coordinate space the engine/client uses.
     *
     * E.g. Given a window resolution of 100x100, and a render resolution of 50x50, a
     * point of 50x50 in screen surface space corresponds to a point of 25,25 in render surface space.
     *
     * This is a more complicated algorithm as it also takes into account how exactly the
     * render is presented/blitted to the screen. In CenterCrop mode, not all of the
     * render space might appear on the screen, and in CenterInside mode, there's screen
     * space which doesn't correspond to a point in render space. Blit mode was abstracted
     * out of the algorithm and replaced by renderBlitRect/screenRenderRect args, which
     * specify which part of the render is used and which part of the screen is rendered to.
     *
     * In situations where the screen point is outside of an area that corresponds to
     * a render point, std::nullopt is returned (e.g. the point corresponds to a black
     * bar region with no render content).
     *
     * @param screenPoint The screen point in question to be converted
     * @param screenBlitRect A rect representing the portion of the screen surface which receives the render
     * @param renderBlitRect A rect representing the portion of the render surface which is presented
     *
     * @return The render surface point corresponding to the provided screen point, or std::nullopt if the point was
     * outside the bounds of the render area.
     */
    [[nodiscard]] NEON_PUBLIC std::optional<NCommon::Point2DReal> ScreenSurfacePointToRenderSurfacePoint(
        const ScreenSurfacePoint& screenPoint,
        const NCommon::RectReal& screenBlitRect,
        const NCommon::RectReal& renderBlitRect);
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_SPACEUTIL_H
