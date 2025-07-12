/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_EVENT_KEYEVENT_H
#define WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_EVENT_KEYEVENT_H

#include "../Key.h"

#include <vector>

namespace Wired::Platform
{
    /**
     * Represents a keyboard key event, as reported by the OS
     */
    struct KeyEvent
    {
        enum class Action
        {
            KeyPress,
            KeyRelease
        };

        KeyEvent(Action _action, PhysicalKeyPair _physicalKey, LogicalKeyPair _logicalKey, std::vector<KeyMod> _keyMods)
            : action(_action)
            , physicalKey(_physicalKey)
            , logicalKey(_logicalKey)
            , keyMods(std::move(_keyMods))
        { }

        Action action;
        PhysicalKeyPair physicalKey;
        LogicalKeyPair logicalKey;
        std::vector<KeyMod> keyMods;
    };
}

#endif //WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_EVENT_KEYEVENT_H
