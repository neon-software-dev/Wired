/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanDescriptorPool.h"
#include "VulkanDebugUtil.h"
#include "VulkanInstance.h"

#include "../Global.h"

#include <NEON/Common/Hash.h>
#include <NEON/Common/Log/ILogger.h>

namespace Wired::GPU
{

std::expected<VulkanDescriptorPool, bool> VulkanDescriptorPool::Create(Global* pGlobal,
                                                                       const uint32_t& descriptorSetLimit,
                                                                       const std::vector<VkDescriptorPoolSize>& descriptorLimits,
                                                                       const VkDescriptorPoolCreateFlags& flags,
                                                                       const std::string& tag)
{
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = (uint32_t)descriptorLimits.size();
    poolInfo.pPoolSizes = descriptorLimits.data();
    poolInfo.maxSets = descriptorSetLimit;
    poolInfo.flags = flags;

    VkDescriptorPool vkDescriptorPool{VK_NULL_HANDLE};
    const auto result = pGlobal->vk.vkCreateDescriptorPool(pGlobal->device.GetVkDevice(), &poolInfo, nullptr, &vkDescriptorPool);
    if (result != VK_SUCCESS)
    {
        pGlobal->pLogger->Error("VulkanDescriptorPool::Create: Call to vkCreateDescriptorPool() failed, result code: {}", (uint32_t)result);
        return std::unexpected(false);
    }

    SetDebugName(pGlobal->vk, pGlobal->device, VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t)vkDescriptorPool, std::format("DescriptorPool-{}", tag));

    return VulkanDescriptorPool(pGlobal, vkDescriptorPool, flags);
}

VulkanDescriptorPool::VulkanDescriptorPool(Global* pGlobal, VkDescriptorPool vkDescriptorPool, VkDescriptorPoolCreateFlags vkDescriptorPoolCreateFlags)
    : m_pGlobal(pGlobal)
    , m_vkDescriptorPool(vkDescriptorPool)
    , m_vkDescriptorPoolCreateFlags(vkDescriptorPoolCreateFlags)
{

}

VulkanDescriptorPool::~VulkanDescriptorPool()
{
    m_pGlobal = nullptr;
    m_vkDescriptorPool = VK_NULL_HANDLE;
    m_vkDescriptorPoolCreateFlags = {};
}

void VulkanDescriptorPool::Destroy()
{
    if (m_vkDescriptorPool != VK_NULL_HANDLE)
    {
        while (!m_allocatedDescriptorSets.empty())
        {
            ReleaseDescriptorSet(m_allocatedDescriptorSets.cbegin()->first, true);
        }

        m_pGlobal->vk.vkDestroyDescriptorPool(m_pGlobal->device.GetVkDevice(), m_vkDescriptorPool, nullptr);
        m_vkDescriptorPool = VK_NULL_HANDLE;
        m_vkDescriptorPoolCreateFlags = {};
    }
}

std::expected<VulkanDescriptorSet, VulkanDescriptorPool::AllocateError> VulkanDescriptorPool::AllocateDescriptorSet(const VulkanDescriptorSetLayout& layout,
                                                                                                                    const std::string& tag)
{
    const std::vector<VkDescriptorSetLayout> layouts = { layout.GetVkDescriptorSetLayout() };

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_vkDescriptorPool;
    allocInfo.descriptorSetCount = (uint32_t)layouts.size();
    allocInfo.pSetLayouts = layouts.data();

    // Adjust the log level before calling vkAllocateDescriptorSets to only log errors; otherwise, when pools
    // run out of memory it'll spam the logs with warnings about it, even though it's a scenario that we
    // gracefully handle (we have our own custom log for it, but at debug level, not warning)
    const auto adjustedLogLevel = ScopedDebugMessengerMinLogLevel(NCommon::LogLevel::Error);

    VkDescriptorSet vkDescriptorSet{VK_NULL_HANDLE};
    const auto result = m_pGlobal->vk.vkAllocateDescriptorSets(m_pGlobal->device.GetVkDevice(), &allocInfo, &vkDescriptorSet);
    if (result != VK_SUCCESS)
    {
        // We handle pool memory errors separately as we by design run pools out of
        // memory and then create new ones as needed; it's not really an error

        if (result == VK_ERROR_OUT_OF_POOL_MEMORY)
        {
            m_pGlobal->pLogger->Debug("VulkanDescriptorPool::AllocateDescriptorSet: Pool ran out of memory: {}", (uint64_t)m_vkDescriptorPool);
            return std::unexpected(AllocateError::OutOfMemory);
        }
        else if (result == VK_ERROR_FRAGMENTED_POOL)
        {
            m_pGlobal->pLogger->Debug("VulkanDescriptorPool::AllocateDescriptorSet: Pool is too fragmented: {}", (uint64_t)m_vkDescriptorPool);
            return std::unexpected(AllocateError::Fragmented);
        }
        else
        {
            m_pGlobal->pLogger->Error("VulkanDescriptorPool::AllocateDescriptorSet: Call to vkAllocateDescriptorSets() failed, result code: {}", (uint32_t)result);
            return std::unexpected(AllocateError::Other);
        }
    }

    SetDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)vkDescriptorSet, std::format("DescriptorSet-{}-{}", tag, (uint64_t)vkDescriptorSet));

    auto descriptorSet = VulkanDescriptorSet(m_pGlobal, vkDescriptorSet);

    m_allocatedDescriptorSets.insert({descriptorSet.GetVkDescriptorSet(), descriptorSet});

    return descriptorSet;
}

void VulkanDescriptorPool::FreeDescriptorSet(const VkDescriptorSet& vkDescriptorSet)
{
    if (!(m_vkDescriptorPoolCreateFlags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT))
    {
        m_pGlobal->pLogger->Error("Attempted to free a descriptor set in a pool that doesn't support it");
        return;
    }

    ReleaseDescriptorSet(vkDescriptorSet, true);
}

void VulkanDescriptorPool::ReleaseDescriptorSet(const VkDescriptorSet& vkDescriptorSet, bool tryToFree)
{
    auto it = m_allocatedDescriptorSets.find(vkDescriptorSet);
    if (it == m_allocatedDescriptorSets.end())
    {
       return;
    }

    // Reclaim memory from the set's debug name
    RemoveDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)vkDescriptorSet);

    // If we were told to try to free the set, and the pool supports freeing individual sets, then free it
    if (tryToFree && (m_vkDescriptorPoolCreateFlags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT))
    {
        m_pGlobal->vk.vkFreeDescriptorSets(m_pGlobal->device.GetVkDevice(), m_vkDescriptorPool, 1, &vkDescriptorSet);
    }

    // Erase our knowledge of the set
    m_allocatedDescriptorSets.erase(it);
}

void VulkanDescriptorPool::ResetPool()
{
    // Release all sets, without trying to free - reclaims debug name memory and releases resources, but then
    // relies on vkResetDescriptorPool below to actually free the set's memory
    while (!m_allocatedDescriptorSets.empty())
    {
        ReleaseDescriptorSet(m_allocatedDescriptorSets.cbegin()->first, false);
    }

    m_pGlobal->vk.vkResetDescriptorPool(m_pGlobal->device.GetVkDevice(), m_vkDescriptorPool, 0);
}

}
