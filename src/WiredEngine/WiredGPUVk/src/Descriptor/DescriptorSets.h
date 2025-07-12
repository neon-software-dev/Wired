/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_DESCRIPTOR_DESCRIPTORSETS_H
#define WIREDENGINE_WIREDGPUVK_SRC_DESCRIPTOR_DESCRIPTORSETS_H

#include "DescriptorPools.h"

#include "../Vulkan/VulkanDescriptorPool.h"

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <mutex>
#include <expected>
#include <optional>
#include <string>
#include <queue>

namespace Wired::GPU
{
    struct Global;

    struct DescriptorSetRequest
    {
        VulkanDescriptorSetLayout descriptorSetLayout;
        SetBindings bindings;
    };

    // TODO Perf: Purge down cached/free sets at some point, no way to reduce cache size at the moment
    class DescriptorSets
    {
        public:

            DescriptorSets(Global* pGlobal, std::string tag);
            ~DescriptorSets();

            void Destroy();

            [[nodiscard]] std::expected<VulkanDescriptorSet, bool> GetVulkanDescriptorSet(const DescriptorSetRequest& request, const std::string& tag);

            void RunCleanUp(bool isIdleCleanUp);

        private:

            using RequestHash = std::size_t;

            struct DescriptorSet
            {
                unsigned int cleanUpsWithoutUse{0};
                VkDescriptorSetLayout vkDescriptorSetLayout{VK_NULL_HANDLE};
                VulkanDescriptorSet vulkanDescriptorSet;
            };

        private:

            [[nodiscard]] static RequestHash GetHash(const DescriptorSetRequest& request);

            void RunCleanUp_CacheUnusedSets();

            void LockDescriptorSetResources(const VulkanDescriptorSet& vulkanDescriptorSet);
            void UnlockDescriptorSetResources(const VulkanDescriptorSet& vulkanDescriptorSet);

        private:

            Global* m_pGlobal;
            std::string m_tag;

            DescriptorPools m_descriptorPools;

            // "Active" descriptor sets which have recently been used and have specific descriptors bound to them
            std::unordered_map<RequestHash, DescriptorSet> m_descriptorSets;

            // "Cached" descriptor sets which haven't recently been used and which no longer have specific descriptors bound
            std::unordered_map<VkDescriptorSetLayout, std::queue<VulkanDescriptorSet>> m_cached;
            std::recursive_mutex m_mutex;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_DESCRIPTOR_DESCRIPTORSETS_H
