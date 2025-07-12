/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_TASK_PRESENTTOSWAPCHAINTASK_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_TASK_PRESENTTOSWAPCHAINTASK_H

#include "RenderTask.h"

#include <Wired/Render/Id.h>

#include <glm/glm.hpp>

#include <optional>

namespace Wired::Render
{
    struct PresentToSwapChainTask : public RenderTask
    {
        [[nodiscard]] Type GetType() const noexcept override { return Type::PresentToSwapChain; }

        std::optional<Render::TextureId> presentTextureId{};
        glm::vec3 clearColor{0};
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_TASK_PRESENTTOSWAPCHAINTASK_H
