/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORMSDL_INCLUDE_WIRED_PLATFORM_SDLEVENTS_H
#define WIREDENGINE_WIREDPLATFORMSDL_INCLUDE_WIRED_PLATFORM_SDLEVENTS_H

#include <Wired/Platform/IEvents.h>

#include <NEON/Common/SharedLib.h>

#include <SDL3/SDL.h>

#include <memory>

namespace Wired::Render
{
    class IRenderer;
}

namespace Wired::Platform
{
    class NEON_PUBLIC SDLEvents : public IEvents
    {
        public:

            explicit SDLEvents(Render::IRenderer* pRenderer);
            ~SDLEvents() override;

            void Initialize(const GPU::ImGuiGlobals& imGuiGlobals) override;

            [[nodiscard]] std::queue<Event> PopEvents() override;

            void RegisterCanRenderCallback(const std::optional<std::function<void(bool)>>& canRenderCallback) override;

            std::optional<std::function<void(bool)>>& GetCanRenderCallback() { return m_canRenderCallback; }

            [[nodiscard]] IKeyboardState* GetKeyboardState() const override;

        private:

            Render::IRenderer* m_pRenderer{nullptr};

            std::unique_ptr<IKeyboardState> m_keyboardState;

            std::optional<std::function<void(bool)>> m_canRenderCallback;
    };
}

#endif //WIREDENGINE_WIREDPLATFORMSDL_INCLUDE_WIRED_PLATFORM_SDLEVENTS_H
