/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Engine/DesktopEngine.h>

#include "DesktopFiles.h"
#include "DesktopSurfaceAccess.h"

#include <Wired/Engine/EngineBuilder.h>

#include <Wired/Render/RendererBuilder.h>

#include <Wired/GPU/WiredGPUVkBuilder.h>
#include <Wired/GPU/WiredGPUVk.h>
#include <Wired/GPU/VulkanSurfaceDetails.h>

#include <Wired/Platform/Platform.h>
#include <Wired/Platform/SDLWindow.h>
#include <Wired/Platform/SDLEvents.h>
#include <Wired/Platform/SDLImage.h>
#include <Wired/Platform/SDLText.h>

#include <NEON/Common/Log/StdLogger.h>
#include <NEON/Common/Metrics/InMemoryMetrics.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>

namespace Wired::Engine
{

DesktopEngine::DesktopEngine() = default;
DesktopEngine::~DesktopEngine() = default;

std::expected<std::vector<std::string>, bool> GetSDLVulkanInstanceExtensions()
{
    uint32_t extensionsCount{0};
    const char* const* pInstanceExtensions = SDL_Vulkan_GetInstanceExtensions(&extensionsCount);
    if (pInstanceExtensions == nullptr)
    {
        return std::unexpected(false);
    }

    std::vector<std::string> instanceExtensions;

    for (unsigned int x = 0; x < extensionsCount; ++x)
    {
        instanceExtensions.emplace_back(pInstanceExtensions[x]);
    }

    return instanceExtensions;
}

PFN_vkGetInstanceProcAddr GetSDLVulkanGetInstanceProcAddr()
{
    return (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
}

bool DesktopEngine::Initialize(const std::string& applicationName,
                               const std::tuple<uint32_t, uint32_t, uint32_t>& applicationVersion,
                               RunMode runMode,
                               NCommon::LogLevel minlogLevel)
{
    m_runMode = runMode;
    m_logger = std::make_unique<NCommon::StdLogger>(minlogLevel);
    m_metrics = std::make_unique<NCommon::InMemoryMetrics>();

    //
    // Initialize SDL video system
    //
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        m_logger->Fatal("DesktopEngine::StartUp: Failed to init SDL Video system");
        return false;
    }

    if (!TTF_Init())
    {
        m_logger->Fatal("DesktopEngine::StartUp: Failed to init SDL TTF system");
        return false;
    }

    //
    // Have SDL load the Vulkan library (if creating an SDL window, do this before that)
    //
    if (!SDL_Vulkan_LoadLibrary(nullptr))
    {
        m_logger->Fatal("DesktopEngine::StartUp: Failed to load Vulkan library. Error: {}", SDL_GetError());
        return false;
    }

    //
    // Now that SDL+Vulkan is set up, ask it for the required instance extensions and for
    // the VkGetInstanceProcAddr function so that we can create a Vulkan instance, but
    // only if we're in windowed mode. Otherwise, we don't care about the extensions SDL
    // returns which will (should?) only be relevant to outputting to a surface.
    //
    std::vector<std::string> requiredInstanceExtensions;

    if (runMode == RunMode::Window)
    {
        const auto sdlInstanceExtensions = GetSDLVulkanInstanceExtensions();
        if (!sdlInstanceExtensions)
        {
            m_logger->Fatal("DesktopEngine::StartUp: GetVkInstanceExtensions failed");
            return false;
        }

        std::ranges::transform(*sdlInstanceExtensions, std::back_inserter(requiredInstanceExtensions), std::identity{});
    }

    // Fetch the Vulkan func that we use to resolve all Vulkan API functions
    const auto pfnVkGetInstanceProcAddr = GetSDLVulkanGetInstanceProcAddr();

    //
    // Create a GPUVk and have it initialize, which primarily just creates a Vulkan instance
    //
    auto gpuVk = GPU::WiredGPUVkBuilder::Build(
        m_logger.get(),
        {
            .applicationName = applicationName,
            .applicationVersion = applicationVersion,
            .requiredInstanceExtensions = requiredInstanceExtensions,
            .supportSurfaceOutput = runMode == RunMode::Window,
            .pfnVkGetInstanceProcAddr = pfnVkGetInstanceProcAddr
        }
    );

    const auto result = gpuVk->Initialize();
    if (!result)
    {
        m_logger->Fatal("DesktopEngine::StartUp: Renderer failed to initialize GPU system");
        return false;
    }
    m_gpu = std::move(gpuVk);

    m_renderer = Render::RendererBuilder::Build(m_logger.get(), m_metrics.get(), m_gpu.get());

    m_initialized = true;

    return true;
}

std::optional<std::vector<std::string>> DesktopEngine::GetSuitablePhysicalDeviceNames() const
{
    return m_gpu->GetSuitablePhysicalDeviceNames();
}

void DesktopEngine::SetRequiredPhysicalDevice(const std::string& physicalDeviceName)
{
    m_gpu->SetRequiredPhysicalDevice(physicalDeviceName);
}

bool ExecWithWindow(NCommon::ILogger* pLogger,
                    NCommon::IMetrics* pMetrics,
                    GPU::WiredGPUVk* pGPU,
                    Render::IRenderer* pRenderer,
                    const std::string& windowTitle,
                    const Platform::SDLWindow::CreateMode& createMode,
                    std::unique_ptr<Client> pClient)
{
    //
    // Open an SDL window
    //
    auto window = std::make_shared<Platform::SDLWindow>(pLogger);
    if (!window->CreateWindow(windowTitle, createMode))
    {
        pLogger->Fatal("DesktopEngine::Exec: Failed to create an SDL window");
        return false;
    }

    // Surface access used by the engine to access/manipulate the window's surface
    const auto surfaceAccess = std::make_unique<DesktopSurfaceAccess>(pLogger, window.get(), pGPU);

    //
    // Setup platform systems
    //
    auto desktopFiles = std::make_unique<Platform::DesktopFiles>(pLogger);
    auto events = std::make_unique<Platform::SDLEvents>(pRenderer);
    auto image = std::make_unique<Platform::SDLImage>(pLogger);
    auto text = std::make_unique<Platform::SDLText>(pLogger);
    auto platform = std::make_unique<Platform::Platform>(window, std::move(events), std::move(desktopFiles), std::move(image), std::move(text));

    //
    // Create the engine and execute it, giving it thread control
    //
    auto engine = Wired::Engine::EngineBuilder::Build(pLogger, pMetrics, surfaceAccess.get(), platform.get(), pRenderer);
    engine->Run(std::move(pClient));

    //
    // Clean Up
    //
    window->DestroyWindow();

    return true;
}

bool DesktopEngine::ExecWindowed(const std::string& windowTitle, const NCommon::Size2DUInt& windowPixelSize, std::unique_ptr<Client> pClient)
{
    if (!m_initialized)
    {
        m_logger->Fatal("DesktopEngine::ExecWindowed: Must call Initialize first");
        return false;
    }

    if (m_runMode != RunMode::Window)
    {
        m_logger->Fatal("DesktopEngine::ExecWindowed: Must call Initialize with a run mode of Window");
        return false;
    }

    return ExecWithWindow(
        m_logger.get(),
        m_metrics.get(),
        m_gpu.get(),
        m_renderer.get(),
        windowTitle,
        Platform::SDLWindow::CreateWindowed{.windowPixelSize = windowPixelSize},
        std::move(pClient)
    );
}

bool DesktopEngine::ExecMaximized(const std::string& windowTitle, std::unique_ptr<Client> pClient)
{
    if (!m_initialized)
    {
        m_logger->Fatal("DesktopEngine::ExecMaximized: Must call Initialize first");
        return false;
    }

    if (m_runMode != RunMode::Window)
    {
        m_logger->Fatal("DesktopEngine::ExecMaximized: Must call Initialize with a run mode of Window");
        return false;
    }

    return ExecWithWindow(
        m_logger.get(),
        m_metrics.get(),
        m_gpu.get(),
        m_renderer.get(),
        windowTitle,
        Platform::SDLWindow::CreateMaximized{},
        std::move(pClient)
    );
}

bool DesktopEngine::ExecFullscreenBorderless(const std::string& windowTitle, std::unique_ptr<Client> pClient)
{
    if (!m_initialized)
    {
        m_logger->Fatal("DesktopEngine::ExecFullscreenBorderless: Must call Initialize first");
        return false;
    }

    if (m_runMode != RunMode::Window)
    {
        m_logger->Fatal("DesktopEngine::ExecFullscreenBorderless: Must call Initialize with a run mode of Window");
        return false;
    }

    return ExecWithWindow(
        m_logger.get(),
        m_metrics.get(),
        m_gpu.get(),
        m_renderer.get(),
        windowTitle,
        Platform::SDLWindow::CreateFullscreenBorderless{},
        std::move(pClient)
    );
}

bool DesktopEngine::ExecHeadless(std::unique_ptr<Client> pClient)
{
    if (!m_initialized)
    {
        m_logger->Fatal("DesktopEngine::ExecHeadless: Must call Initialize first");
        return false;
    }

    if (m_runMode != RunMode::Headless)
    {
        m_logger->Fatal("DesktopEngine::ExecHeadless: Must call Initialize with a run mode of Headless");
        return false;
    }

    //
    // Setup platform systems
    //
    auto desktopFiles = std::make_unique<Platform::DesktopFiles>(m_logger.get());
    auto window = std::make_shared<Platform::SDLWindow>(m_logger.get());
    auto events = std::make_unique<Platform::SDLEvents>( m_renderer.get());
    auto image = std::make_unique<Platform::SDLImage>(m_logger.get());
    auto text = std::make_unique<Platform::SDLText>(m_logger.get());
    auto platform = std::make_unique<Platform::Platform>(window, std::move(events), std::move(desktopFiles), std::move(image), std::move(text));

    //
    // Create the engine and execute it, giving it thread control
    //
    auto engine = Wired::Engine::EngineBuilder::Build(m_logger.get(), m_metrics.get(), std::nullopt, platform.get(),  m_renderer.get());
    engine->Run(std::move(pClient));

    return true;
}

void DesktopEngine::Destroy()
{
    // Destroy the GPU
    if (m_gpu)
    {
        m_gpu->Destroy();
        m_gpu = nullptr;
    }

    // Unload/quit SDL systems
    SDL_Vulkan_UnloadLibrary();
    TTF_Quit();
    SDL_Quit();

    m_initialized = false;
    m_runMode = {};
    m_logger = nullptr;
    m_metrics = nullptr;
    m_renderer = nullptr;
}

}
