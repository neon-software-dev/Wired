/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_EVENT_MOUSEBUTTONEVENT_H
#define WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_EVENT_MOUSEBUTTONEVENT_H

#include "../Mouse.h"

#include <cstdint>

namespace Wired::Platform
{
    /**
     * Represents a mouse button event, as reported by the OS
     */
    struct MouseButtonEvent
    {
        MouseButtonEvent(uint32_t _mouseId,
                         MouseButton _button,
                         ClickType _clickType,
                         uint32_t _clicks,
                         float _xPos,
                         float _yPos)
            : mouseId(_mouseId)
            , button(_button)
            , clickType(_clickType)
            , clicks(_clicks)
            , xPos(_xPos)
            , yPos(_yPos)
        { }

        uint32_t mouseId;       // Unique id of the mouse, in multi-mouse setups
        MouseButton button;     // The mouse button in question
        ClickType clickType;    // Whether the mouse button was pressed or released
        uint32_t clicks;        // 1 for single-click, 2 for double-click, etc.

        // These positions are in screen surface space within the engine, and then overwritten
        // with virtual space positions before being passed to the scene. WARNING! These positions
        // do not have any camera transformation applied.
        float xPos;
        float yPos;
    };
}

#endif //WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_EVENT_MOUSEBUTTONEVENT_H
