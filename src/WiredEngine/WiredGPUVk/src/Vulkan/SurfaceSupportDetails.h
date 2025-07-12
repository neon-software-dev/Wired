/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKAN_SURFACESUPPORTDETAILS_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKAN_SURFACESUPPORTDETAILS_H

#include "VulkanPhysicalDevice.h"
#include "VulkanSurface.h"

namespace Wired::GPU
{
    struct Global;

    class SurfaceSupportDetails
    {
        public:

            [[nodiscard]] static SurfaceSupportDetails Fetch(Global* pGlobal,
                                                             const VulkanPhysicalDevice& physicalDevice,
                                                             const VulkanSurface& surface);

            VkSurfaceCapabilitiesKHR capabilities{};
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;

        private:

            SurfaceSupportDetails() = default;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKAN_SURFACESUPPORTDETAILS_H
