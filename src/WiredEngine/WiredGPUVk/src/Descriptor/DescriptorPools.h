/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_DESCRIPTOR_DESCRIPTORPOOLS_H
#define WIREDENGINE_WIREDGPUVK_SRC_DESCRIPTOR_DESCRIPTORPOOLS_H

#include "../Vulkan/VulkanDescriptorSet.h"
#include "../Vulkan/VulkanDescriptorSetLayout.h"
#include "../Vulkan/VulkanDescriptorPool.h"

#include <expected>
#include <string>
#include <unordered_map>
#include <mutex>

namespace Wired::GPU
{
    struct Global;

    // TODO: Clean up fragmented pools
    class DescriptorPools
    {
        public:

            explicit DescriptorPools(Global* pGlobal);
            ~DescriptorPools();

            void Destroy();

            [[nodiscard]] std::expected<VulkanDescriptorSet, bool> AllocateDescriptorSet(const VulkanDescriptorSetLayout& layout, const std::string& tag);
            void FreeDescriptorSet(const VkDescriptorSet& vkDescriptorSet);

        private:

            enum PoolState
            {
                Untapped,   // We should still attempt to allocate from this pool
                Tapped,     // A previous call to this pool to allocate returned out of memory
                Fragmented  // A previous call to this pool to allocate returned fragmented
            };

            struct DescriptorPool
            {
                VulkanDescriptorPool pool;
                PoolState state{PoolState::Untapped};
            };

        private:

            [[nodiscard]] std::expected<VulkanDescriptorSet, VulkanDescriptorPool::AllocateError>
                AllocateDescriptorSet(DescriptorPool& descriptorPool, const VulkanDescriptorSetLayout& layout, const std::string& tag);

        private:

            Global* m_pGlobal;

            std::unordered_map<VkDescriptorPool, DescriptorPool> m_pools;
            std::unordered_map<VkDescriptorSet, VkDescriptorPool> m_setToPool;
            VkDescriptorPool m_activePool{VK_NULL_HANDLE};
            std::mutex m_poolsMutex;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_DESCRIPTOR_DESCRIPTORPOOLS_H
