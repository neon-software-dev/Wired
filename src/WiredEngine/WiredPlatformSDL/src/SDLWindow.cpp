/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Platform/SDLWindow.h>

#include <NEON/Common/Log/ILogger.h>

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_mouse.h>

#include <string>

namespace Wired::Platform
{

SDLWindow::SDLWindow(const NCommon::ILogger* pLogger)
    : m_pLogger(pLogger)
{

}

SDLWindow::~SDLWindow()
{
    m_pLogger = nullptr;
}

bool SDLWindow::CreateWindow(std::string_view windowTitle, const CreateMode& createMode)
{
    if (m_window)
    {
        DestroyWindow();
    }

    //
    // Create an SDL window and claim it for the GPU device
    //
    int windowWidth = 0;
    int windowHeight = 0;
    SDL_WindowFlags windowFlags{SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY};

    if (std::holds_alternative<CreateWindowed>(createMode))
    {
        const auto& v = std::get<CreateWindowed>(createMode);
        windowWidth = static_cast<int>(v.windowPixelSize.GetWidth());
        windowHeight = static_cast<int>(v.windowPixelSize.GetHeight());
        windowFlags |= SDL_WINDOW_RESIZABLE;

        LogInfo("SDLWindow::CreateWindow: Creating a windowed SDL window: {}x{}", windowWidth, windowHeight);
    }
    else if (std::holds_alternative<CreateMaximized>(createMode))
    {
        windowFlags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;

        LogInfo("SDLWindow::CreateWindow: Creating a maximized SDL window");
    }
    else if (std::holds_alternative<CreateFullscreenBorderless>(createMode))
    {
        windowFlags |= SDL_WINDOW_FULLSCREEN;

        LogInfo("SDLWindow::CreateWindow: Creating a fullscreen borderless SDL window");
    }

    auto pWindow = SDL_CreateWindow(windowTitle.data(), windowWidth, windowHeight, windowFlags);
    if (pWindow == nullptr)
    {
        LogFatal("SDLWindow::CreateWindow: SDL_CreateWindow() - Failed to create window. Error: {}", SDL_GetError());
        return false;
    }

    m_window = pWindow;

    return true;
}

void SDLWindow::DestroyWindow()
{
    LogInfo("SDLWindow: Destroying SDL window");

    if (m_window)
    {
        SDL_DestroyWindow(*m_window);
        m_window = std::nullopt;
    }
}

std::expected<NCommon::Size2DUInt, bool> SDLWindow::GetWindowPixelSize() const
{
    if (!m_window)
    {
        LogError("SDLWindow::GetWindowPixelSize: No window exists");
        return std::unexpected(false);
    }

    int width{0};
    int height{0};

    if (!SDL_GetWindowSizeInPixels(*m_window, &width, &height))
    {
        LogError("SDLWindow::GetWindowPixelSize: No window exists");
        return std::unexpected(false);
    }

    if (width < 0 || height < 0)
    {
        LogError("SDLWindow::GetWindowPixelSize: Width or height reported as negative: {}x{}", width, height);
        return std::unexpected(false);
    }

    return NCommon::Size2DUInt(static_cast<unsigned int>(width), static_cast<unsigned int>(height));
}

GPU::ShaderBinaryType SDLWindow::GetShaderBinaryType() const
{
    return GPU::ShaderBinaryType::SPIRV;
}

void SDLWindow::SetMouseCapture(bool doCaptureMouse) const
{
    if (!m_window) { return; }

    SDL_SetWindowRelativeMouseMode(*m_window, doCaptureMouse);
}

bool SDLWindow::IsCapturingMouse() const
{
    if (!m_window) { return false; }

    return SDL_GetWindowRelativeMouseMode(*m_window);
}

}
