/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANQUERYPOOL_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANQUERYPOOL_H

#include <vulkan/vulkan.h>

#include <expected>
#include <cstdint>
#include <string>

namespace Wired::GPU
{
    struct Global;

    class VulkanQueryPool
    {
        public:

            [[nodiscard]] static bool QueueFamilySupportsTimestampQueries(Global* pGlobal, uint32_t queueFamilyIndex);

            [[nodiscard]] static std::expected<VulkanQueryPool, bool> Create(Global* pGlobal, uint32_t numTimestamps, const std::string& tag);

        public:

            VulkanQueryPool() = default;
            VulkanQueryPool(Global* pGlobal, uint32_t numTimestamps, VkQueryPool vkQueryPool);
            ~VulkanQueryPool();

            void Destroy();

            [[nodiscard]] VkQueryPool GetVkQueryPool() const noexcept { return m_vkQueryPool; }
            [[nodiscard]] uint32_t GetNumTimestamps() const noexcept { return m_numTimestamps; }

        private:

            Global* m_pGlobal{nullptr};
            uint32_t m_numTimestamps{0};
            VkQueryPool m_vkQueryPool{VK_NULL_HANDLE};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANQUERYPOOL_H
