/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "DescriptorPools.h"

#include "../Global.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::GPU
{

DescriptorPools::DescriptorPools(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

DescriptorPools::~DescriptorPools()
{
    m_pGlobal = nullptr;
}

void DescriptorPools::Destroy()
{
    std::lock_guard<std::mutex> lock(m_poolsMutex);

    for (auto& it : m_pools)
    {
        it.second.pool.Destroy();
    }
    m_pools.clear();
    m_setToPool.clear();
    m_activePool = VK_NULL_HANDLE;
}

std::expected<VulkanDescriptorSet, bool>
DescriptorPools::AllocateDescriptorSet(const VulkanDescriptorSetLayout& layout, const std::string& tag)
{
    std::lock_guard<std::mutex> lock(m_poolsMutex);

    std::expected<VulkanDescriptorSet, VulkanDescriptorPool::AllocateError> allocatedSet;

    //
    // Try to allocate from the active pool, if one exists
    //
    if (m_activePool != VK_NULL_HANDLE)
    {
        allocatedSet = AllocateDescriptorSet(m_pools.at(m_activePool), layout, tag);
        if (allocatedSet)
        {
            return *allocatedSet;
        }
    }

    //
    // Loop through all untapped pools, trying to allocate from them
    //
    for (auto& it : m_pools)
    {
        // Only untapped pools
        if (it.second.state != PoolState::Untapped) { continue; }

        // Already tried the active pool above
        if (it.second.pool.GetVkDescriptorPool() == m_activePool) { continue; }

        allocatedSet = AllocateDescriptorSet(it.second, layout, tag);
        if (allocatedSet)
        {
            // Mark this pool as the active pool
            m_activePool = it.second.pool.GetVkDescriptorPool();
            return *allocatedSet;
        }
    }

    //
    // If here, then we have no pools that can allocate, so create a new pool
    // TODO Perf: Adjust limits
    //
    const std::vector<VkDescriptorPoolSize> descriptorLimits = {
        { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1000 },
        { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, .descriptorCount = 10 },
        { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1000 },
        { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1000 }
    };

    const auto newVulkanDescriptorPool = VulkanDescriptorPool::Create(
        m_pGlobal,
        1000,
        descriptorLimits,
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        std::format("{}", layout.GetTag())
    );
    if (!newVulkanDescriptorPool)
    {
        m_pGlobal->pLogger->Error("DescriptorPools::AllocateDescriptorSet: Failed to create new descriptor pool");
        return std::unexpected(false);
    }

    m_pools.insert({newVulkanDescriptorPool->GetVkDescriptorPool(), DescriptorPool{.pool = *newVulkanDescriptorPool, .state = PoolState::Untapped}});

    //
    // Allocate from the new pool
    //
    allocatedSet = AllocateDescriptorSet(m_pools.at(newVulkanDescriptorPool->GetVkDescriptorPool()), layout, tag);
    if (!allocatedSet)
    {
        m_pGlobal->pLogger->Error("DescriptorPools::AllocateDescriptorSet: Failed to allocate from fresh descriptor pool");
        return std::unexpected(false);
    }

    // Mark the new pool as the active pool
    m_activePool = newVulkanDescriptorPool->GetVkDescriptorPool();
    return *allocatedSet;
}

std::expected<VulkanDescriptorSet, VulkanDescriptorPool::AllocateError> DescriptorPools::AllocateDescriptorSet(
    DescriptorPools::DescriptorPool& descriptorPool,
    const VulkanDescriptorSetLayout& layout,
    const std::string& tag)
{
    if (descriptorPool.state != PoolState::Untapped)
    {
        return std::unexpected(VulkanDescriptorPool::AllocateError::Other);
    }

    const auto descriptorSet = descriptorPool.pool.AllocateDescriptorSet(layout, tag);
    if (!descriptorSet)
    {
        switch (descriptorSet.error())
        {
            case VulkanDescriptorPool::AllocateError::OutOfMemory: { descriptorPool.state = PoolState::Tapped; } break;
            case VulkanDescriptorPool::AllocateError::Fragmented: { descriptorPool.state = PoolState::Fragmented; } break;
            case VulkanDescriptorPool::AllocateError::Other: { /* no-op */ } break;
        }

        return std::unexpected(descriptorSet.error());
    }

    m_setToPool.insert({descriptorSet->GetVkDescriptorSet(), descriptorPool.pool.GetVkDescriptorPool()});

    return *descriptorSet;
}

void DescriptorPools::FreeDescriptorSet(const VkDescriptorSet& vkDescriptorSet)
{
    const auto it = m_setToPool.find(vkDescriptorSet);
    if (it == m_setToPool.cend())
    {
        m_pGlobal->pLogger->Error("DescriptorPools::FreeDescriptorSet: No set to pool mapping exists for: {}", (uint64_t)vkDescriptorSet);
        return;
    }

    auto& descriptorPool = m_pools.at(it->second);

    descriptorPool.pool.FreeDescriptorSet(vkDescriptorSet);

    // Since the pool had a descriptor set freed, moved it back to untapped, so we can try to
    // allocate from it again in the future
    descriptorPool.state = PoolState::Untapped;
}

}
