/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDDESKTOP_SRC_DESKTOPSURFACEACCESS_H
#define WIREDENGINE_WIREDDESKTOP_SRC_DESKTOPSURFACEACCESS_H

#include <Wired/Engine/ISurfaceAccess.h>

#include <vulkan/vulkan.h>

namespace NCommon
{
    class ILogger;
}

namespace Wired::GPU
{
    class WiredGPUVk;
}

namespace Wired
{
    namespace Platform
    {
        class SDLWindow;
    }

    class DesktopSurfaceAccess : public Engine::ISurfaceAccess
    {
        public:

            DesktopSurfaceAccess(const NCommon::ILogger* pLogger, Platform::SDLWindow* pSDLWindow, GPU::WiredGPUVk* pGPUVk);
            ~DesktopSurfaceAccess() override;

            [[nodiscard]] bool CreateSurface() override;
            void DestroySurface() override;
            [[nodiscard]] std::optional<std::unique_ptr<GPU::SurfaceDetails>> GetSurfaceDetails() const override;

            // TODO: Is it more appropriate to move these to Platform somewhere?
            bool InitImGuiForSurface(const GPU::ImGuiGlobals& imGuiGlobals) override;
            void StartImGuiFrame() override;
            void DestroyImGuiForSurface() override;

        private:

            const NCommon::ILogger* m_pLogger;
            Platform::SDLWindow* m_pSDLWindow;
            GPU::WiredGPUVk* m_pGPUVk;

            VkSurfaceKHR m_vkSurface{VK_NULL_HANDLE};
    };
}

#endif //WIREDENGINE_WIREDDESKTOP_SRC_DESKTOPSURFACEACCESS_H
