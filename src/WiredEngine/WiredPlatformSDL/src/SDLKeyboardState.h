/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORMSDL_SRC_SDLKEYBOARDSTATE_H
#define WIREDENGINE_WIREDPLATFORMSDL_SRC_SDLKEYBOARDSTATE_H

#include <Wired/Platform/IKeyboardState.h>

namespace Wired::Platform
{
    class SDLKeyboardState : public IKeyboardState
    {
        public:

            //
            // IKeyboardState
            //
            [[nodiscard]] bool IsPhysicalKeyPressed(const Platform::PhysicalKey& physicalKey) const override;
            [[nodiscard]] bool IsPhysicalKeyPressed(const Platform::ScanCode& scanCode) const override;
            [[nodiscard]] bool IsLogicalKeyPressed(const Platform::LogicalKey& logicalKey) const override;
            [[nodiscard]] bool IsModifierPressed(const Platform::KeyMod& keyMod) const override;
    };
}

#endif //WIREDENGINE_WIREDPLATFORMSDL_SRC_SDLKEYBOARDSTATE_H
