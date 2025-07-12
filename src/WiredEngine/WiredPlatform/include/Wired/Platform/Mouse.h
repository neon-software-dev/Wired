/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_MOUSE_H
#define WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_MOUSE_H

namespace Wired::Platform
{
    enum class MouseButton
    {
        Left,
        Middle,
        Right,
        X1,
        X2
    };

    enum class ClickType
    {
        Press,
        Release
    };
}

#endif //WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_MOUSE_H
