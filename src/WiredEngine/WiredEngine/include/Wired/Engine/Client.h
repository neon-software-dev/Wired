/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_CLIENT_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_CLIENT_H

#include "EventListener.h"
#include "EngineCommon.h"

#include <Wired/Engine/Render/EngineRenderTask.h>

#include <NEON/Common/SharedLib.h>

#include <optional>

namespace Wired::Engine
{
    class NEON_PUBLIC Client : public EventListener
    {
        public:

            ~Client() override = default;

            virtual void OnClientStart(IEngineAccess* pEngine);
            virtual void OnClientStop() {};

            [[nodiscard]] virtual std::optional<std::vector<std::shared_ptr<EngineRenderTask>>> GetRenderTasks() const { return std::nullopt; }
            [[nodiscard]] virtual bool OnRecordImGuiCommands() { return false; }

        protected:

            IEngineAccess* engine{nullptr};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_CLIENT_H
