/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANDEVICE_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANDEVICE_H

#include "VulkanPhysicalDevice.h"
#include "VulkanSurface.h"

#include <vulkan/vulkan.h>

#include <expected>
#include <optional>

namespace Wired::GPU
{
    struct Global;

    struct DeviceCreateResult
    {
        VkDevice vkDevice{VK_NULL_HANDLE};

        VkQueue vkCommandQueue{VK_NULL_HANDLE};
        uint32_t commandQueueFamilyIndex{0};

        std::optional<VkQueue> vkPresentQueue;
        std::optional<uint32_t> presentQueueFamilyIndex{0};
    };

    class VulkanDevice
    {
        public:

            [[nodiscard]] static std::expected<DeviceCreateResult, bool> Create(Global* pGlobal);

        public:

            VulkanDevice() = default;
            VulkanDevice(Global* pGlobal, VkDevice vkDevice);
            ~VulkanDevice();

            void Destroy();

            [[nodiscard]] VkDevice GetVkDevice() const noexcept { return m_vkDevice; }

        private:

            Global* m_pGlobal{nullptr};
            VkDevice m_vkDevice{VK_NULL_HANDLE};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANDEVICE_H
