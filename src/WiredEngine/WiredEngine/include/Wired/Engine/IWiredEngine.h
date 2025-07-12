/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_IWIREDENGINE_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_IWIREDENGINE_H

#include "Client.h"

#include <NEON/Common/SharedLib.h>
#include <NEON/Common/ImageData.h>

#include <memory>
#include <optional>

namespace Wired::Engine
{
    class NEON_PUBLIC IWiredEngine
    {
        public:

            virtual ~IWiredEngine() = default;

            /**
             * Pass thread control to the engine's run loop, executing the provided logic.
             */
            virtual void Run(std::unique_ptr<Client> pClient) = 0;

            /**
             * Thread-safe method which returns the most recently rendered frame as an image,
             * if the engine is configured with a render graph that headless renders. Returns
             * std::nullopt if the engine isn't initialized yet, or hasn't rendered a frame in
             * headless mode.
             */
            [[nodiscard]] virtual std::optional<std::shared_ptr<NCommon::ImageData>> GetRenderOutput() const = 0;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_IWIREDENGINE_H
