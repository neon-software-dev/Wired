/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERABLE_SPRITERENDERABLE_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERABLE_SPRITERENDERABLE_H

#include "../Id.h"

#include <NEON/Common/Space/Rect.h>
#include <NEON/Common/Space/Point3D.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <optional>

namespace Wired::Render
{
    struct SpriteRenderable
    {
        SpriteId id;

        TextureId textureId;
        NCommon::Point3DReal position{0,0,0};
        glm::quat orientation{};
        glm::vec3 scale{1.0f};
        std::optional<NCommon::RectReal> srcPixelRect{}; // Optional pixel subset of the source texture to use. If not set, will use the entire texture.
        std::optional<NCommon::Size2DReal> dstSize{}; // Optional destination size. If not set, will match the pixel size of the source being used.
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERABLE_SPRITERENDERABLE_H
