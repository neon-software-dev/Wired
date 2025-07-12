/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_RENDER_ENGINERENDERTASK_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_RENDER_ENGINERENDERTASK_H

namespace Wired::Engine
{
    class EngineRenderTask
    {
        public:

            enum class Type
            {
                RenderWorld,
                PresentToSwapChain
            };

        public:

            virtual ~EngineRenderTask() = default;

            [[nodiscard]] virtual Type GetType() const noexcept = 0;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_RENDER_ENGINERENDERTASK_H
