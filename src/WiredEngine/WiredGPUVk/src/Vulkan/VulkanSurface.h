/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANSURFACE_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANSURFACE_H

#include <NEON/Common/Space/Size2D.h>

#include <vulkan/vulkan.h>

#include <utility>
#include <functional>
#include <expected>

namespace Wired::GPU
{
    struct Global;

    class VulkanSurface
    {
        public:

            VulkanSurface() = default;
            VulkanSurface(Global* pGlobal, VkSurfaceKHR vkSurface, const NCommon::Size2DUInt& pixelSize);
            ~VulkanSurface();

            [[nodiscard]] VkSurfaceKHR GetVkSurface() const noexcept { return m_vkSurface; }

            [[nodiscard]] std::expected<NCommon::Size2DUInt, bool> GetSurfacePixelSize() const;

        private:

            Global* m_pGlobal{nullptr};
            VkSurfaceKHR m_vkSurface{nullptr};
            NCommon::Size2DUInt m_pixelSize;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANSURFACE_H
