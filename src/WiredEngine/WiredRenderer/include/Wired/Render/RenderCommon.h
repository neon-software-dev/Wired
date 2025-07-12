/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERCOMMON_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERCOMMON_H

#include <NEON/Common/Space/Point2D.h>
#include <NEON/Common/Space/Point3D.h>

namespace Wired::Render
{
    // Warning: These are matched to GPU indexing of cube faces, don't reorder them
    enum class CubeFace
    {
        Right, Left, Up, Down, Back, Forward
    };

    [[nodiscard]] inline glm::vec2 ToGLM(const NCommon::Point2DReal& point)
    {
        return {point.x, point.y};
    }

    [[nodiscard]] inline glm::vec3 ToGLM(const NCommon::Point3DReal& point)
    {
        return {point.x, point.y, point.z};
    }
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERCOMMON_H
