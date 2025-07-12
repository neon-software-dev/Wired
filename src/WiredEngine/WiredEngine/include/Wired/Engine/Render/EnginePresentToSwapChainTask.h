/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_RENDER_ENGINEPRESENTTOSWAPCHAINTASK_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_RENDER_ENGINEPRESENTTOSWAPCHAINTASK_H

#include "EngineRenderTask.h"

#include <Wired/Render/Id.h>

#include <glm/glm.hpp>

#include <optional>

namespace Wired::Engine
{
    struct EnginePresentToSwapChainTask : public EngineRenderTask
    {
        [[nodiscard]] Type GetType() const noexcept override { return Type::PresentToSwapChain; }

        std::optional<Render::TextureId> presentTextureId{};
        glm::vec3 clearColor{0};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_RENDER_ENGINEPRESENTTOSWAPCHAINTASK_H
