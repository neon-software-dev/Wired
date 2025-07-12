/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Render/RendererBuilder.h>

#include "Renderer.h"

namespace Wired::Render
{

std::unique_ptr<IRenderer> RendererBuilder::Build(const NCommon::ILogger* pLogger, NCommon::IMetrics* pMetrics, GPU::WiredGPU* pGPU)
{
    return std::make_unique<Renderer>(pLogger, pMetrics, pGPU);
}

}
