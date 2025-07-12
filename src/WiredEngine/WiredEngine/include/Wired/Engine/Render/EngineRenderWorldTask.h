/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_RENDER_ENGINERENDERWORLDTASK_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_RENDER_ENGINERENDERWORLDTASK_H

#include "EngineRenderTask.h"

#include "../World/WorldCommon.h"

#include <Wired/Render/Id.h>

#include <glm/glm.hpp>

#include <string>
#include <optional>
#include <vector>

namespace Wired::Engine
{
    struct EngineRenderWorldTask : public EngineRenderTask
    {
        [[nodiscard]] Type GetType() const noexcept override { return Type::RenderWorld; }

        std::string worldName;

        std::vector<Render::TextureId> targetColorTextureIds{};
        glm::vec3 clearColor{0};

        std::optional<Render::TextureId> targetDepthTextureId{};

        CameraId worldCameraId{};
        CameraId spriteCameraId{};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_RENDER_ENGINERENDERWORLDTASK_H
