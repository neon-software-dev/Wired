/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANDESCRIPTORPOOL_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANDESCRIPTORPOOL_H

#include "VulkanDescriptorSet.h"
#include "VulkanDescriptorSetLayout.h"

#include <vulkan/vulkan.h>

#include <expected>
#include <vector>
#include <string>
#include <unordered_map>

namespace Wired::GPU
{
    struct Global;

    class VulkanDescriptorPool
    {
        public:

            enum class AllocateError
            {
                OutOfMemory,
                Fragmented,
                Other
            };

        public:

            [[nodiscard]] static std::expected<VulkanDescriptorPool, bool> Create(Global* pGlobal,
                                                                                  const uint32_t& descriptorSetLimit,
                                                                                  const std::vector<VkDescriptorPoolSize>& descriptorLimits,
                                                                                  const VkDescriptorPoolCreateFlags& flags,
                                                                                  const std::string& tag);

        public:

            VulkanDescriptorPool() = default;
            VulkanDescriptorPool(Global* pGlobal, VkDescriptorPool vkDescriptorPool, VkDescriptorPoolCreateFlags vkDescriptorPoolCreateFlags);
            ~VulkanDescriptorPool();

            void Destroy();

            [[nodiscard]] VkDescriptorPool GetVkDescriptorPool() const { return m_vkDescriptorPool; }

            [[nodiscard]] std::expected<VulkanDescriptorSet, AllocateError> AllocateDescriptorSet(const VulkanDescriptorSetLayout& layout,
                                                                                                  const std::string& tag);

            /**
             * Free the specified descriptor set, reclaiming its memory. This pool must have
             * been created with the VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT flag.
             */
            void FreeDescriptorSet(const VkDescriptorSet& vkDescriptorSet);

            void ResetPool();

        private:

            void ReleaseDescriptorSet(const VkDescriptorSet& vkDescriptorSet, bool tryToFree);

        private:

            Global* m_pGlobal{nullptr};
            VkDescriptorPool m_vkDescriptorPool{VK_NULL_HANDLE};
            VkDescriptorPoolCreateFlags m_vkDescriptorPoolCreateFlags{};

            std::unordered_map<VkDescriptorSet, VulkanDescriptorSet> m_allocatedDescriptorSets;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANDESCRIPTORPOOL_H
