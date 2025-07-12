/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_ENGINECOMMON_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_ENGINECOMMON_H

#include <NEON/Common/Id.h>
#include <NEON/Common/Space/Size2D.h>
#include <NEON/Common/Space/Rect.h>
#include <NEON/Common/Space/Point2D.h>
#include <NEON/Common/Space/Point3D.h>

#include <glm/glm.hpp>

#include <vector>
#include <memory>

namespace Wired::Engine
{
    DEFINE_INTEGRAL_ID_TYPE(ModelId)

    /**
     * A 3D point in virtual space - a right-handed coordinate system with
     * +X to the right, +Y upwards, and +Z out of the screen.
     */
    struct VirtualSpacePoint : public NCommon::Point3DReal {
        using NCommon::Point3DReal::Point3DReal;
    };

    /**
     * A size, in virtual space
     */
    struct VirtualSpaceSize : public NCommon::Size2DReal{
        using NCommon::Size2DReal::Size2D;
    };

    /**
     * A 2D point in virtual surface space - a coordinate system with
     * +X to the right and +Y downwards, and the origin in the top-left
     */
    struct VirtualSurfacePoint : public NCommon::Point2DReal {
        using NCommon::Point2DReal::Point2DReal;
    };

    /**
     * A rect within a virtual surface space - a coordinate system with
     * +X to the right and +Y downwards, and the origin in the top-left
     */
    struct VirtualSurfaceRect : public NCommon::RectReal {
        using NCommon::RectReal::Rect;
    };

    /**
     * A 2D point in screen surface space - a coordinate system with
     * +X to the right and +Y downwards.
     */
    struct ScreenSurfacePoint : public NCommon::Point2DReal {
        using NCommon::Point2DReal::Point2DReal;
    };
}

DEFINE_INTEGRAL_ID_HASH(Wired::Engine::ModelId)

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_ENGINECOMMON_H
