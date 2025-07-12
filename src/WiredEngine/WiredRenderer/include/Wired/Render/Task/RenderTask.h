/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_TASK_RENDERTASK_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_TASK_RENDERTASK_H

namespace Wired::Render
{
    class RenderTask
    {
        public:

            enum class Type
            {
                RenderGroup,
                PresentToSwapChain
            };

        public:

            virtual ~RenderTask() = default;

            [[nodiscard]] virtual Type GetType() const noexcept = 0;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_TASK_RENDERTASK_H
