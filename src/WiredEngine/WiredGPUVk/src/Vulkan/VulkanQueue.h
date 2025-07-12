/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANQUEUE_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANQUEUE_H

#include "VulkanCommandBuffer.h"

#include "../Util/SyncPrimitives.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <optional>
#include <string>

namespace Wired::GPU
{
    struct Global;

    class VulkanQueue
    {
        public:

            [[nodiscard]] static VulkanQueue CreateFrom(Global* pGlobal,
                                                        VkQueue vkQueue,
                                                        uint32_t queueFamilyIndex,
                                                        const std::string& tag);

        public:

            VulkanQueue() = default;
            VulkanQueue(Global* pGlobal, VkQueue vkQueue, uint32_t queueFamilyIndex, std::string tag);
            ~VulkanQueue();

            void Destroy();

            [[nodiscard]] VkQueue GetVkQueue() const noexcept { return m_vkQueue; }
            [[nodiscard]] uint32_t GetQueueFamilyIndex() const noexcept { return m_queueFamilyIndex; }

            [[nodiscard]] bool SubmitBatch(const std::vector<VulkanCommandBuffer>& commandBuffers,
                                           const WaitOn& waitOn,
                                           const SignalOn& signalOn,
                                           const std::optional<VkFence>& vkFence,
                                           const std::string& submitTag);

        private:

            Global* m_pGlobal{nullptr};
            VkQueue m_vkQueue{VK_NULL_HANDLE};
            uint32_t m_queueFamilyIndex{0};
            std::string m_tag;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANQUEUE_H
