/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_EVENT_EVENTS_H
#define WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_EVENT_EVENTS_H

#include "KeyEvent.h"
#include "MouseButtonEvent.h"
#include "MouseMoveEvent.h"

#include <variant>

namespace Wired::Platform
{
    struct EventQuit{};
    struct EventWindowShown{};
    struct EventWindowHidden{};

    using Event = std::variant<
        EventQuit,
        EventWindowShown,
        EventWindowHidden,
        KeyEvent,
        MouseButtonEvent,
        MouseMoveEvent
    >;
}

#endif //WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_EVENT_EVENTS_H
