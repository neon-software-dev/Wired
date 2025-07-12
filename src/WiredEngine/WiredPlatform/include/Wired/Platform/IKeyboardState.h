/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_IKEYBOARDSTATE_H
#define WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_IKEYBOARDSTATE_H

#include "Key.h"

namespace Wired::Platform
{
    /**
     * User-facing interface to answering keyboard related queries
     */
    class IKeyboardState
    {
        public:

            virtual ~IKeyboardState() = default;

            [[nodiscard]] virtual bool IsPhysicalKeyPressed(const Platform::PhysicalKey& physicalKey) const = 0;
            [[nodiscard]] virtual bool IsPhysicalKeyPressed(const Platform::ScanCode& scanCode) const = 0;
            [[nodiscard]] virtual bool IsLogicalKeyPressed(const Platform::LogicalKey& logicalKey) const = 0;
            [[nodiscard]] virtual bool IsModifierPressed(const Platform::KeyMod& keyMod) const = 0;
    };
}

#endif //WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_IKEYBOARDSTATE_H
