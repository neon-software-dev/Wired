/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_ENGINEBUILDER_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_ENGINEBUILDER_H

#include "IWiredEngine.h"
#include "ISurfaceAccess.h"

#include <Wired/Platform/IPlatform.h>

#include <Wired/Render/IRenderer.h>

#include <NEON/Common/SharedLib.h>
#include <NEON/Common/Log/ILogger.h>
#include <NEON/Common/Metrics/IMetrics.h>

#include <memory>

namespace Wired::Engine
{
    class NEON_PUBLIC EngineBuilder
    {
        public:

            [[nodiscard]] static std::unique_ptr<IWiredEngine> Build(
                NCommon::ILogger* pLogger,
                NCommon::IMetrics* pMetrics,
                const std::optional<ISurfaceAccess*>& pSurfaceAccess,
                Platform::IPlatform* pPlatform,
                Render::IRenderer* pRenderer
            );
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_ENGINEBUILDER_H
