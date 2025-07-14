/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "DesktopSurfaceAccess.h"

#include <Wired/Engine/EngineImGui.h>

#include <Wired/Platform/SDLWindow.h>

#include <Wired/GPU/WiredGPUVk.h>
#include <Wired/GPU/VulkanSurfaceDetails.h>

#include <NEON/Common/Log/ILogger.h>

#include <SDL3/SDL_vulkan.h>

#include <imgui_impl_sdl3.h>

namespace Wired
{


DesktopSurfaceAccess::DesktopSurfaceAccess(const NCommon::ILogger* pLogger, Platform::SDLWindow* pSDLWindow, GPU::WiredGPUVk* pGPUVk)
    : m_pLogger(pLogger)
    , m_pSDLWindow(pSDLWindow)
    , m_pGPUVk(pGPUVk)
{ }

DesktopSurfaceAccess::~DesktopSurfaceAccess()
{
    m_pLogger = nullptr;
    m_pSDLWindow = nullptr;
    m_pGPUVk = nullptr;
}

bool DesktopSurfaceAccess::CreateSurface()
{
    LogInfo("DesktopSurfaceAccess: Creating surface");

    const auto pWindow = m_pSDLWindow->GetSDLWindow();
    if (!pWindow)
    {
        LogFatal("DesktopSurfaceAccess::CreateSurface: Can't create a surface if no window exists");
        return false;
    }

    if (!SDL_Vulkan_CreateSurface(*pWindow, m_pGPUVk->GetVkInstance(), nullptr, &m_vkSurface))
    {
        LogFatal("DesktopSurfaceAccess::CreateSurface: SDL_Vulkan_CreateSurface() call failed");
        return false;
    }

    return true;
}

void DesktopSurfaceAccess::DestroySurface()
{
    LogInfo("DesktopSurfaceAccess: Destroying surface");

    if (m_vkSurface != VK_NULL_HANDLE)
    {
        SDL_Vulkan_DestroySurface(m_pGPUVk->GetVkInstance(), m_vkSurface, nullptr);
        m_vkSurface = VK_NULL_HANDLE;
    }
}

std::optional<std::unique_ptr<GPU::SurfaceDetails>> DesktopSurfaceAccess::GetSurfaceDetails() const
{
    const auto windowPixelSize = m_pSDLWindow->GetWindowPixelSize();
    if (!windowPixelSize)
    {
        LogError("DesktopSurfaceAccess::GetSurfaceDetails: No window currently exists");
        return std::nullopt;
    }

    std::unique_ptr<GPU::VulkanSurfaceDetails> surfaceDetails = std::make_unique<GPU::VulkanSurfaceDetails>();
    surfaceDetails->pixelSize = *windowPixelSize;
    surfaceDetails->vkSurface = m_vkSurface;

    return surfaceDetails;
}

bool DesktopSurfaceAccess::InitImGuiForSurface(const GPU::ImGuiGlobals& imGuiGlobals)
{
    #ifdef WIRED_IMGUI
        ImGui::SetCurrentContext(imGuiGlobals.pImGuiContext);
        ImGui::SetAllocatorFunctions(imGuiGlobals.pImGuiMemAllocFunc, imGuiGlobals.pImGuiMemFreeFunc, nullptr);

        return ImGui_ImplSDL3_InitForVulkan(*m_pSDLWindow->GetSDLWindow());
    #else
        (void)imGuiGlobals;
        return false;
    #endif
}

void DesktopSurfaceAccess::StartImGuiFrame()
{
    #ifdef WIRED_IMGUI
        ImGui_ImplSDL3_NewFrame();
    #endif
}

void DesktopSurfaceAccess::DestroyImGuiForSurface()
{
    #ifdef WIRED_IMGUI
        ImGui_ImplSDL3_Shutdown();
    #endif
}

}
