/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Engine/EngineBuilder.h>

#include "WiredEngine.h"

namespace Wired::Engine
{

std::unique_ptr<IWiredEngine> Wired::Engine::EngineBuilder::Build(NCommon::ILogger* pLogger,
                                                                  NCommon::IMetrics* pMetrics,
                                                                  const std::optional<ISurfaceAccess*>& pSurfaceAccess,
                                                                  Platform::IPlatform* pPlatform,
                                                                  Render::IRenderer* pRenderer)
{
    return std::make_unique<WiredEngine>(pLogger, pMetrics, pSurfaceAccess, pPlatform, pRenderer);
}

}
