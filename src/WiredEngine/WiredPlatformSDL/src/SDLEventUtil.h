/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORMSDL_SRC_SDLEVENTUTIL_H
#define WIREDENGINE_WIREDPLATFORMSDL_SRC_SDLEVENTUTIL_H

#include <Wired/Platform/Key.h>
#include <Wired/Platform/Event/KeyEvent.h>
#include <Wired/Platform/Event/MouseButtonEvent.h>
#include <Wired/Platform/Event/MouseMoveEvent.h>

#include <NEON/Common/Build.h>

#include <SDL3/SDL.h>

#include <optional>

namespace Wired::Platform
{
    SUPPRESS_IS_NOT_USED
    [[nodiscard]] static MouseButtonEvent ParseSDLMouseButtonEvent(const SDL_Event& event)
    {
        MouseButton button{MouseButton::Left};

        switch (event.button.button)
        {
            case SDL_BUTTON_LEFT:   button = MouseButton::Left; break;
            case SDL_BUTTON_MIDDLE: button = MouseButton::Middle; break;
            case SDL_BUTTON_RIGHT:  button = MouseButton::Right; break;
            case SDL_BUTTON_X1:     button = MouseButton::X1; break;
            case SDL_BUTTON_X2:     button = MouseButton::X2; break;
        }

        ClickType clickType = ClickType::Press;
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) { clickType = ClickType::Release; }

        return MouseButtonEvent{event.button.which, button, clickType, event.button.clicks, event.button.x, event.button.y};
    }

    SUPPRESS_IS_NOT_USED
    [[nodiscard]] static MouseMoveEvent ParseMouseMoveEvent(const SDL_Event& sdlEvent) noexcept
    {
        return MouseMoveEvent{
            sdlEvent.motion.which,
            (float)sdlEvent.motion.x,
            (float)sdlEvent.motion.y,
            (float)sdlEvent.motion.xrel,
            (float)sdlEvent.motion.yrel
        };
    }

    SUPPRESS_IS_NOT_USED
    [[nodiscard]] static std::optional<KeyEvent> ParseSDLKeyEvent(const SDL_Event& event)
    {
        //
        // Action
        //
        KeyEvent::Action action{KeyEvent::Action::KeyPress};

        switch(event.type)
        {
            case SDL_EVENT_KEY_DOWN: action = KeyEvent::Action::KeyPress; break;
            case SDL_EVENT_KEY_UP: action = KeyEvent::Action::KeyRelease; break;
            default: return std::nullopt;
        }

        //
        // Physical Key
        //
        PhysicalKeyPair physicalKeyPair{PhysicalKey::Unknown, (Platform::ScanCode)event.key.scancode};

        switch (event.key.scancode)
        {
            case SDL_SCANCODE_ESCAPE: physicalKeyPair.key = PhysicalKey::Escape; break;
            case SDL_SCANCODE_LCTRL: physicalKeyPair.key = PhysicalKey::LControl; break;
            case SDL_SCANCODE_RCTRL: physicalKeyPair.key = PhysicalKey::RControl; break;
            case SDL_SCANCODE_LSHIFT: physicalKeyPair.key = PhysicalKey::LShift; break;
            case SDL_SCANCODE_RSHIFT: physicalKeyPair.key = PhysicalKey::RShift; break;
            case SDL_SCANCODE_BACKSPACE: physicalKeyPair.key = PhysicalKey::Backspace; break;
            case SDL_SCANCODE_KP_ENTER: physicalKeyPair.key = PhysicalKey::KeypadEnter; break;
            case SDL_SCANCODE_RETURN: physicalKeyPair.key = PhysicalKey::Return; break;
            case SDL_SCANCODE_A: physicalKeyPair.key = PhysicalKey::A; break;
            case SDL_SCANCODE_B: physicalKeyPair.key = PhysicalKey::B; break;
            case SDL_SCANCODE_C: physicalKeyPair.key = PhysicalKey::C; break;
            case SDL_SCANCODE_D: physicalKeyPair.key = PhysicalKey::D; break;
            case SDL_SCANCODE_E: physicalKeyPair.key = PhysicalKey::E; break;
            case SDL_SCANCODE_F: physicalKeyPair.key = PhysicalKey::F; break;
            case SDL_SCANCODE_G: physicalKeyPair.key = PhysicalKey::G; break;
            case SDL_SCANCODE_H: physicalKeyPair.key = PhysicalKey::H; break;
            case SDL_SCANCODE_I: physicalKeyPair.key = PhysicalKey::I; break;
            case SDL_SCANCODE_J: physicalKeyPair.key = PhysicalKey::J; break;
            case SDL_SCANCODE_K: physicalKeyPair.key = PhysicalKey::K; break;
            case SDL_SCANCODE_L: physicalKeyPair.key = PhysicalKey::L; break;
            case SDL_SCANCODE_M: physicalKeyPair.key = PhysicalKey::M; break;
            case SDL_SCANCODE_N: physicalKeyPair.key = PhysicalKey::N; break;
            case SDL_SCANCODE_O: physicalKeyPair.key = PhysicalKey::O; break;
            case SDL_SCANCODE_P: physicalKeyPair.key = PhysicalKey::P; break;
            case SDL_SCANCODE_Q: physicalKeyPair.key = PhysicalKey::Q; break;
            case SDL_SCANCODE_R: physicalKeyPair.key = PhysicalKey::R; break;
            case SDL_SCANCODE_S: physicalKeyPair.key = PhysicalKey::S; break;
            case SDL_SCANCODE_T: physicalKeyPair.key = PhysicalKey::T; break;
            case SDL_SCANCODE_U: physicalKeyPair.key = PhysicalKey::U; break;
            case SDL_SCANCODE_V: physicalKeyPair.key = PhysicalKey::V; break;
            case SDL_SCANCODE_W: physicalKeyPair.key = PhysicalKey::W; break;
            case SDL_SCANCODE_X: physicalKeyPair.key = PhysicalKey::X; break;
            case SDL_SCANCODE_Y: physicalKeyPair.key = PhysicalKey::Y; break;
            case SDL_SCANCODE_Z: physicalKeyPair.key = PhysicalKey::Z; break;
            case SDL_SCANCODE_1: physicalKeyPair.key = PhysicalKey::_1; break;
            case SDL_SCANCODE_2: physicalKeyPair.key = PhysicalKey::_2; break;
            case SDL_SCANCODE_3: physicalKeyPair.key = PhysicalKey::_3; break;
            case SDL_SCANCODE_4: physicalKeyPair.key = PhysicalKey::_4; break;
            case SDL_SCANCODE_5: physicalKeyPair.key = PhysicalKey::_5; break;
            case SDL_SCANCODE_6: physicalKeyPair.key = PhysicalKey::_6; break;
            case SDL_SCANCODE_7: physicalKeyPair.key = PhysicalKey::_7; break;
            case SDL_SCANCODE_8: physicalKeyPair.key = PhysicalKey::_8; break;
            case SDL_SCANCODE_9: physicalKeyPair.key = PhysicalKey::_9; break;
            case SDL_SCANCODE_0: physicalKeyPair.key = PhysicalKey::_0; break;
            case SDL_SCANCODE_SPACE: physicalKeyPair.key = PhysicalKey::Space; break;
            case SDL_SCANCODE_PERIOD: physicalKeyPair.key = PhysicalKey::Period; break;
            case SDL_SCANCODE_SLASH: physicalKeyPair.key = PhysicalKey::Slash; break;
            case SDL_SCANCODE_COMMA: physicalKeyPair.key = PhysicalKey::Comma; break;
            case SDL_SCANCODE_GRAVE: physicalKeyPair.key = PhysicalKey::Grave; break;
            case SDL_SCANCODE_MINUS: physicalKeyPair.key = PhysicalKey::Minus; break;
            default: break;
        }

        //
        // Logical Key
        //
        LogicalKeyPair logicalKeyPair{LogicalKey::Unknown, (uint32_t)event.key.key};

        switch (event.key.key)
        {
            case SDLK_ESCAPE: logicalKeyPair.key = LogicalKey::Escape; break;
            case SDLK_LCTRL:
            case SDLK_RCTRL: logicalKeyPair.key = LogicalKey::Control; break;
            case SDLK_LSHIFT:
            case SDLK_RSHIFT: logicalKeyPair.key = LogicalKey::Shift; break;
            case SDLK_BACKSPACE: logicalKeyPair.key = LogicalKey::Backspace; break;
            case SDLK_KP_ENTER: logicalKeyPair.key = LogicalKey::Enter; break;
            case SDLK_RETURN: logicalKeyPair.key = LogicalKey::Return; break;
            case SDLK_A: logicalKeyPair.key = LogicalKey::A; break;
            case SDLK_B: logicalKeyPair.key = LogicalKey::B; break;
            case SDLK_C: logicalKeyPair.key = LogicalKey::C; break;
            case SDLK_D: logicalKeyPair.key = LogicalKey::D; break;
            case SDLK_E: logicalKeyPair.key = LogicalKey::E; break;
            case SDLK_F: logicalKeyPair.key = LogicalKey::F; break;
            case SDLK_G: logicalKeyPair.key = LogicalKey::G; break;
            case SDLK_H: logicalKeyPair.key = LogicalKey::H; break;
            case SDLK_I: logicalKeyPair.key = LogicalKey::I; break;
            case SDLK_J: logicalKeyPair.key = LogicalKey::J; break;
            case SDLK_K: logicalKeyPair.key = LogicalKey::K; break;
            case SDLK_L: logicalKeyPair.key = LogicalKey::L; break;
            case SDLK_M: logicalKeyPair.key = LogicalKey::M; break;
            case SDLK_N: logicalKeyPair.key = LogicalKey::N; break;
            case SDLK_O: logicalKeyPair.key = LogicalKey::O; break;
            case SDLK_P: logicalKeyPair.key = LogicalKey::P; break;
            case SDLK_Q: logicalKeyPair.key = LogicalKey::Q; break;
            case SDLK_R: logicalKeyPair.key = LogicalKey::R; break;
            case SDLK_S: logicalKeyPair.key = LogicalKey::S; break;
            case SDLK_T: logicalKeyPair.key = LogicalKey::T; break;
            case SDLK_U: logicalKeyPair.key = LogicalKey::U; break;
            case SDLK_V: logicalKeyPair.key = LogicalKey::V; break;
            case SDLK_W: logicalKeyPair.key = LogicalKey::W; break;
            case SDLK_X: logicalKeyPair.key = LogicalKey::X; break;
            case SDLK_Y: logicalKeyPair.key = LogicalKey::Y; break;
            case SDLK_Z: logicalKeyPair.key = LogicalKey::Z; break;
            case SDLK_1: logicalKeyPair.key = LogicalKey::_1; break;
            case SDLK_2: logicalKeyPair.key = LogicalKey::_2; break;
            case SDLK_3: logicalKeyPair.key = LogicalKey::_3; break;
            case SDLK_4: logicalKeyPair.key = LogicalKey::_4; break;
            case SDLK_5: logicalKeyPair.key = LogicalKey::_5; break;
            case SDLK_6: logicalKeyPair.key = LogicalKey::_6; break;
            case SDLK_7: logicalKeyPair.key = LogicalKey::_7; break;
            case SDLK_8: logicalKeyPair.key = LogicalKey::_8; break;
            case SDLK_9: logicalKeyPair.key = LogicalKey::_9; break;
            case SDLK_0: logicalKeyPair.key = LogicalKey::_0; break;
            case SDLK_SPACE: logicalKeyPair.key = LogicalKey::Space; break;
            case SDLK_PERIOD: logicalKeyPair.key = LogicalKey::Period; break;
            case SDLK_SLASH:
            case SDLK_QUESTION: logicalKeyPair.key = LogicalKey::Slash; break;
            case SDLK_COMMA: logicalKeyPair.key = LogicalKey::Comma; break;
            case SDLK_GRAVE: logicalKeyPair.key = LogicalKey::Grave; break;
            case SDLK_MINUS:
            case SDLK_UNDERSCORE: logicalKeyPair.key = LogicalKey::Minus; break;
        }

        //
        // Modifiers
        //
        std::vector<KeyMod> keyMod;

        if (event.key.mod & SDL_KMOD_LSHIFT) { keyMod.push_back(KeyMod::Shift); }
        if (event.key.mod & SDL_KMOD_RSHIFT) { keyMod.push_back(KeyMod::Shift); }
        if (event.key.mod & SDL_KMOD_LCTRL) { keyMod.push_back(KeyMod::Control); }
        if (event.key.mod & SDL_KMOD_LCTRL) { keyMod.push_back(KeyMod::Control); }

        return KeyEvent(action, physicalKeyPair, logicalKeyPair, keyMod);
    }

    SUPPRESS_IS_NOT_USED
    [[nodiscard]] static std::optional<ScanCode> PhysicalKeyToScanCode(const PhysicalKey& physicalKey)
    {
        switch (physicalKey)
        {
            case PhysicalKey::Unknown: return std::nullopt;
            case PhysicalKey::A: return SDL_SCANCODE_A;
            case PhysicalKey::B: return SDL_SCANCODE_B;
            case PhysicalKey::C: return SDL_SCANCODE_C;
            case PhysicalKey::D: return SDL_SCANCODE_D;
            case PhysicalKey::E: return SDL_SCANCODE_E;
            case PhysicalKey::F: return SDL_SCANCODE_F;
            case PhysicalKey::G: return SDL_SCANCODE_G;
            case PhysicalKey::H: return SDL_SCANCODE_H;
            case PhysicalKey::I: return SDL_SCANCODE_I;
            case PhysicalKey::J: return SDL_SCANCODE_J;
            case PhysicalKey::K: return SDL_SCANCODE_K;
            case PhysicalKey::L: return SDL_SCANCODE_L;
            case PhysicalKey::M: return SDL_SCANCODE_M;
            case PhysicalKey::N: return SDL_SCANCODE_N;
            case PhysicalKey::O: return SDL_SCANCODE_O;
            case PhysicalKey::P: return SDL_SCANCODE_P;
            case PhysicalKey::Q: return SDL_SCANCODE_Q;
            case PhysicalKey::R: return SDL_SCANCODE_R;
            case PhysicalKey::S: return SDL_SCANCODE_S;
            case PhysicalKey::T: return SDL_SCANCODE_T;
            case PhysicalKey::U: return SDL_SCANCODE_U;
            case PhysicalKey::V: return SDL_SCANCODE_V;
            case PhysicalKey::W: return SDL_SCANCODE_W;
            case PhysicalKey::X: return SDL_SCANCODE_X;
            case PhysicalKey::Y: return SDL_SCANCODE_Y;
            case PhysicalKey::Z: return SDL_SCANCODE_Z;
            case PhysicalKey::_1: return SDL_SCANCODE_1;
            case PhysicalKey::_2: return SDL_SCANCODE_2;
            case PhysicalKey::_3: return SDL_SCANCODE_3;
            case PhysicalKey::_4: return SDL_SCANCODE_4;
            case PhysicalKey::_5: return SDL_SCANCODE_5;
            case PhysicalKey::_6: return SDL_SCANCODE_6;
            case PhysicalKey::_7: return SDL_SCANCODE_7;
            case PhysicalKey::_8: return SDL_SCANCODE_8;
            case PhysicalKey::_9: return SDL_SCANCODE_9;
            case PhysicalKey::_0: return SDL_SCANCODE_0;
            case PhysicalKey::KeypadEnter: return SDL_SCANCODE_KP_ENTER;
            case PhysicalKey::Return: return SDL_SCANCODE_RETURN;
            case PhysicalKey::Escape: return SDL_SCANCODE_ESCAPE;
            case PhysicalKey::Backspace: return SDL_SCANCODE_BACKSPACE;
            case PhysicalKey::Tab: return SDL_SCANCODE_TAB;
            case PhysicalKey::Space: return SDL_SCANCODE_SPACE;
            case PhysicalKey::Minus: return SDL_SCANCODE_MINUS;
            case PhysicalKey::Grave: return SDL_SCANCODE_GRAVE;
            case PhysicalKey::Comma: return SDL_SCANCODE_COMMA;
            case PhysicalKey::Period: return SDL_SCANCODE_PERIOD;
            case PhysicalKey::Slash: return SDL_SCANCODE_SLASH;
            case PhysicalKey::LControl: return SDL_SCANCODE_LCTRL;
            case PhysicalKey::RControl: return SDL_SCANCODE_RCTRL;
            case PhysicalKey::LShift: return SDL_SCANCODE_LSHIFT;
            case PhysicalKey::RShift: return SDL_SCANCODE_RSHIFT;
        }

        return std::nullopt;
    }
}

#endif //WIREDENGINE_WIREDPLATFORMSDL_SRC_SDLEVENTUTIL_H
