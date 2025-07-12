/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORMSDL_INCLUDE_WIRED_PLATFORM_SDLWINDOW_H
#define WIREDENGINE_WIREDPLATFORMSDL_INCLUDE_WIRED_PLATFORM_SDLWINDOW_H

#include <Wired/Platform/IWindow.h>

#include <NEON/Common/SharedLib.h>
#include <NEON/Common/Space/Size2D.h>

#include <string>
#include <unordered_set>
#include <optional>
#include <variant>
#include <vector>

struct SDL_Window;

namespace NCommon
{
    class ILogger;
}

namespace Wired::Platform
{
    class NEON_PUBLIC SDLWindow : public IWindow
    {
        public:

            struct CreateWindowed
            {
                NCommon::Size2DUInt windowPixelSize{};
            };

            struct CreateMaximized
            {

            };

            struct CreateFullscreenBorderless
            {

            };

            using CreateMode = std::variant<CreateWindowed, CreateMaximized, CreateFullscreenBorderless>;

        public:

            explicit SDLWindow(const NCommon::ILogger* pLogger);

            ~SDLWindow() override;

            //
            // IWindow
            //
            [[nodiscard]] std::expected<NCommon::Size2DUInt, bool> GetWindowPixelSize() const override;
            [[nodiscard]] GPU::ShaderBinaryType GetShaderBinaryType() const override;
            void SetMouseCapture(bool doCaptureMouse) const override;
            [[nodiscard]] bool IsCapturingMouse() const override;

            //
            // Internal
            //
            [[nodiscard]] bool CreateWindow(std::string_view windowTitle, const CreateMode& createMode);
            void DestroyWindow();

            [[nodiscard]] std::optional<SDL_Window*> GetSDLWindow() const noexcept { return m_window; }

        private:

            const NCommon::ILogger* m_pLogger{nullptr};

            std::optional<SDL_Window*> m_window;
    };
}

#endif //WIREDENGINE_WIREDPLATFORMSDL_INCLUDE_WIRED_PLATFORM_SDLWINDOW_H
