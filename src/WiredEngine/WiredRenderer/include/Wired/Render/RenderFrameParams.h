/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERFRAMEPARAMS_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERFRAMEPARAMS_H

#include "StateUpdate.h"

#include "Task/RenderTask.h"

#include <vector>
#include <memory>
#include <optional>

struct ImDrawData;

namespace Wired::Render
{
    struct RenderFrameParams
    {
        std::vector<StateUpdate> stateUpdates;
        std::vector<std::shared_ptr<RenderTask>> renderTasks;
        std::optional<ImDrawData*> imDrawData;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERFRAMEPARAMS_H
