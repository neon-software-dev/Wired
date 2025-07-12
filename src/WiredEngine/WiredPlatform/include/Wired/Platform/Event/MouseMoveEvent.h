/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_EVENT_MOUSEMOVEEVENT_H
#define WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_EVENT_MOUSEMOVEEVENT_H

#include <cstdint>
#include <optional>

namespace Wired::Platform
{
    /**
     * Represents a mouse movement event, as reported by the OS
     */
    struct MouseMoveEvent
    {
        /**
        * @param pointerId The ID of the pointer/mouse being moved
        * @param xPos The new x position of the mouse, in pixels. (Note that this value is in virtual space).
        * @param yPos The new y position of the mouse, in pixels. (Note that this value is in virtual space).
        * @param xRel How far the mouse's x position changed, in pixels
        * @param yRel How far the mouse's y position changed, in pixels
        */
        MouseMoveEvent(uint64_t _pointerId, float _xPos, float _yPos, float _xRel, float _yRel)
            : pointerId(_pointerId)
            , xPos(_xPos)
            , yPos(_yPos)
            , xRel(_xRel)
            , yRel(_yRel)
        { }

        uint64_t pointerId; // Unique id of the mouse, in multi-mouse setups

        // These positions are in screen surface space within the engine, and overwritten
        // with virtual space positions before they're passed to the scene. If the mouse
        // movement is outside the virtual space then they will be set to std::nullopt.
        // Warning: These positions do not have any camera transformation applied.
        std::optional<float> xPos;
        std::optional<float> yPos;

        // Warning: These values are render surface space pixels, not virtual surface space
        float xRel;
        float yRel;
    };
}

#endif //WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_EVENT_MOUSEMOVEEVENT_H
