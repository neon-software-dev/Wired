/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_TASK_RENDERGROUPTASK_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_TASK_RENDERGROUPTASK_H

#include "RenderTask.h"

#include "../Id.h"
#include "../Camera.h"

#include <glm/glm.hpp>

#include <string>
#include <optional>
#include <vector>

namespace Wired::Render
{
    struct RenderGroupTask : public RenderTask
    {
        [[nodiscard]] Type GetType() const noexcept override { return Type::RenderGroup; }

        std::string groupName;

        std::vector<Render::TextureId> targetColorTextureIds{};
        glm::vec3 clearColor{0};

        std::optional<Render::TextureId> targetDepthTextureId{};

        Camera worldCamera{};
        Camera spriteCamera{};

        std::optional<TextureId> skyBoxTextureId;
        std::optional<glm::mat4> skyBoxTransform;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_TASK_RENDERGROUPTASK_H
