/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_IEVENTS_H
#define WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_IEVENTS_H

#include "Event/Events.h"

#include <Wired/GPU/ImGuiGlobals.h>

#include <queue>
#include <functional>
#include <optional>

namespace Wired::Platform
{
    class IKeyboardState;

    class IEvents
    {
        public:

            virtual ~IEvents() = default;

            /**
             * Called by the engine during its StartUp flow
             */
            virtual void Initialize(const std::optional<GPU::ImGuiGlobals>& imGuiGlobals) = 0;

            [[nodiscard]] virtual std::queue<Event> PopEvents() = 0;

            /**
             * Registers a callback that the event system can call *from a random thread*, notifying the engine of
             * whether it should or shouldn't be executing rendering code. Only used on mobile, where lifecycle events
             * can be received from SDL on a random thread and need to be immediately processed so that we don't execute
             * rendering code when the app activity/window/surface is not renderable.
             */
            virtual void RegisterCanRenderCallback(const std::optional<std::function<void(bool)>>& canRenderCallback) = 0;

            [[nodiscard]] virtual IKeyboardState* GetKeyboardState() const = 0;
    };
}

#endif //WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_IEVENTS_H
