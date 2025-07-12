/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_ISURFACEACCESS_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_ISURFACEACCESS_H

#include <Wired/Engine/IEngineAccess.h>

#include <Wired/GPU/SurfaceDetails.h>
#include <Wired/GPU/ImGuiGlobals.h>

#include <NEON/Common/Space/Size2D.h>

#include <optional>
#include <memory>

namespace Wired::Engine
{
    class ISurfaceAccess
    {
        public:

            virtual ~ISurfaceAccess() = default;

            [[nodiscard]] virtual bool CreateSurface() = 0;
            virtual void DestroySurface() = 0;

            [[nodiscard]] virtual bool InitImGuiForSurface(const GPU::ImGuiGlobals& imGuiGlobals) = 0;
            virtual void StartImGuiFrame() = 0;
            virtual void DestroyImGuiForSurface() = 0;

            [[nodiscard]] virtual std::optional<std::unique_ptr<GPU::SurfaceDetails>> GetSurfaceDetails() const = 0;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_ISURFACEACCESS_H
