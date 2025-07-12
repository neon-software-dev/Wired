/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDDESKTOP_SRC_DESKTOPENGINE_H
#define WIREDENGINE_WIREDDESKTOP_SRC_DESKTOPENGINE_H

#include <Wired/Engine/Client.h>

#include <NEON/Common/SharedLib.h>

#include <string>
#include <utility>
#include <cstdint>
#include <memory>
#include <vector>

namespace NCommon
{
    class ILogger;
    class IMetrics;
}

namespace Wired::Render
{
    class IRenderer;
}

namespace Wired::GPU
{
    class WiredGPUVk;
}

namespace Wired::Engine
{
    class Client;

    enum class RunMode
    {
        Window,
        Headless
    };

    class NEON_PUBLIC DesktopEngine
    {
        public:

            DesktopEngine();
            ~DesktopEngine();

            [[nodiscard]] bool Initialize(const std::string& applicationName,
                                          const std::tuple<uint32_t, uint32_t, uint32_t>& applicationVersion,
                                          RunMode runMode);
            void Destroy();

            //
            // Available after successful Initialize call
            //
            [[nodiscard]] std::optional<std::vector<std::string>> GetSuitablePhysicalDeviceNames() const;
            void SetRequiredPhysicalDevice(const std::string& physicalDeviceName);

            bool ExecWindowed(const std::string& windowTitle, const NCommon::Size2DUInt& windowPixelSize, std::unique_ptr<Client> pClient);
            bool ExecMaximized(const std::string& windowTitle, std::unique_ptr<Client> pClient);
            bool ExecFullscreenBorderless(const std::string& windowTitle, std::unique_ptr<Client> pClient);
            bool ExecHeadless(std::unique_ptr<Client> pClient);

        private:

            bool m_initialized{false};
            RunMode m_runMode{};
            std::unique_ptr<NCommon::ILogger> m_logger;
            std::unique_ptr<NCommon::IMetrics> m_metrics;
            std::unique_ptr<GPU::WiredGPUVk> m_gpu;
            std::unique_ptr<Render::IRenderer> m_renderer;
    };
}

#endif //WIREDENGINE_WIREDDESKTOP_SRC_DESKTOPENGINE_H
