/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANCOMMANDPOOL_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANCOMMANDPOOL_H

#include "VulkanCommandBuffer.h"

#include <vulkan/vulkan.h>

#include <expected>
#include <string>
#include <unordered_set>

namespace Wired::GPU
{
    struct Global;

    class VulkanCommandPool
    {
        public:

            [[nodiscard]] static std::expected<VulkanCommandPool, bool> Create(Global* pGlobal,
                                                                               const uint32_t& queueFamilyIndex,
                                                                               const VkCommandPoolCreateFlags& vkCreateFlags,
                                                                               const std::string& tag);

        public:

            VulkanCommandPool() = default;
            VulkanCommandPool(Global* pGlobal, VkCommandPool vkCommandPool, VkCommandPoolCreateFlags vkCreateFlags);
            ~VulkanCommandPool();

            void Destroy();

            [[nodiscard]] VkCommandPool GetVkCommandPool() const noexcept { return m_vkCommandPool; }

            [[nodiscard]] std::expected<VulkanCommandBuffer, bool> AllocateCommandBuffer(CommandBufferType type, const std::string& tag);
            void FreeCommandBuffer(const VulkanCommandBuffer& commandBuffer);
            void FreeAllCommandBuffers();

            void ResetCommandBuffer(const VulkanCommandBuffer& commandBuffer, bool trimMemory);
            void ResetPool(bool trimMemory);

        private:

            Global* m_pGlobal{nullptr};
            VkCommandPool m_vkCommandPool{VK_NULL_HANDLE};
            VkCommandPoolCreateFlags m_vkCreateFlags{};

            std::unordered_set<VulkanCommandBuffer, VulkanCommandBuffer::Hash> m_allocatedCommandBuffers;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANCOMMANDPOOL_H
