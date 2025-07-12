/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERERBUILDER_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERERBUILDER_H

#include "IRenderer.h"

#include <NEON/Common/SharedLib.h>

namespace NCommon
{
    class ILogger;
    class IMetrics;
}

namespace Wired::GPU
{
    class WiredGPU;
}

namespace Wired::Render
{
    class NEON_PUBLIC RendererBuilder
    {
        public:

            [[nodiscard]] static std::unique_ptr<IRenderer> Build(const NCommon::ILogger* pLogger, NCommon::IMetrics* pMetrics, GPU::WiredGPU* pGPU);
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERERBUILDER_H
