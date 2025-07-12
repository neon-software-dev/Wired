/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SDLKeyboardState.h"

#include "SDLEventUtil.h"

#include <cassert>

namespace Wired::Platform
{

bool SDLKeyboardState::IsPhysicalKeyPressed(const Platform::PhysicalKey& physicalKey) const
{
    const auto scancode = PhysicalKeyToScanCode(physicalKey);
    if (!scancode)
    {
        return false;
    }

    return IsPhysicalKeyPressed(*scancode);
}

bool SDLKeyboardState::IsPhysicalKeyPressed(const Platform::ScanCode& scanCode) const
{
    int numKeys = 0;
    const auto keyState = SDL_GetKeyboardState(&numKeys);

    if (scanCode >= (unsigned int)numKeys)
    {
        return false;
    }

    return keyState[scanCode] == 1;
}

bool SDLKeyboardState::IsLogicalKeyPressed(const LogicalKey& logicalKey) const
{
    (void)logicalKey;
    // TODO
    assert(false);
    return false;
}

bool SDLKeyboardState::IsModifierPressed(const KeyMod& keyMod) const
{
    switch (keyMod)
    {
        case KeyMod::Control: return IsPhysicalKeyPressed(PhysicalKey::LControl) || IsPhysicalKeyPressed(PhysicalKey::RControl);
        case KeyMod::Shift: return IsPhysicalKeyPressed(PhysicalKey::LShift) || IsPhysicalKeyPressed(PhysicalKey::RShift);
    }

    return false;
}

}
