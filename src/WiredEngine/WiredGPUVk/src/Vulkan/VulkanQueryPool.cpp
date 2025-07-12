/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanQueryPool.h"
#include "VulkanDebugUtil.h"

#include "../Global.h"

namespace Wired::GPU
{

bool VulkanQueryPool::QueueFamilySupportsTimestampQueries(Global* pGlobal, uint32_t queueFamilyIndex)
{
    const auto physicalDeviceLimits = pGlobal->physicalDevice.GetPhysicalDeviceProperties().properties.limits;
    if (NCommon::AreEqual(physicalDeviceLimits.timestampPeriod, 0.0f))
    {
        return false;
    }

    // If timestamps aren't supported across all queues, we need to check individual queue for timestamp support
    if (!physicalDeviceLimits.timestampComputeAndGraphics)
    {
        const auto queueFamilyProperties = pGlobal->physicalDevice.GetQueueFamilyProperties();
        if (queueFamilyIndex >= queueFamilyProperties.size())
        {
            return false;
        }

        if (queueFamilyProperties.at(queueFamilyIndex).timestampValidBits == 0)
        {
            return false;
        }
    }

    return true;
}

std::expected<VulkanQueryPool, bool> VulkanQueryPool::Create(Global* pGlobal, uint32_t numTimestamps, const std::string& tag)
{
    VkQueryPool vkQueryPool{VK_NULL_HANDLE};

    VkQueryPoolCreateInfo vkQueryPoolCreateInfo{};
    vkQueryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    vkQueryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    vkQueryPoolCreateInfo.queryCount = numTimestamps;
    const auto result = pGlobal->vk.vkCreateQueryPool(pGlobal->device.GetVkDevice(), &vkQueryPoolCreateInfo, nullptr, &vkQueryPool);
    if (result != VK_SUCCESS)
    {
        pGlobal->pLogger->Error("VulkanQueryPool::Create: Call to vkCreateQueryPool failed, error: {}", (uint32_t)result);
        return std::unexpected(false);
    }

    SetDebugName(pGlobal->vk, pGlobal->device, VK_OBJECT_TYPE_QUERY_POOL, (uint64_t)vkQueryPool, std::format("QueryPool-{}", tag));

    return VulkanQueryPool(pGlobal, numTimestamps, vkQueryPool);
}

VulkanQueryPool::VulkanQueryPool(Global* pGlobal, uint32_t numTimestamps, VkQueryPool vkQueryPool)
    : m_pGlobal(pGlobal)
    , m_numTimestamps(numTimestamps)
    , m_vkQueryPool(vkQueryPool)
{

}

VulkanQueryPool::~VulkanQueryPool()
{
    m_pGlobal = nullptr;
    m_numTimestamps = 0;
    m_vkQueryPool = VK_NULL_HANDLE;
}

void VulkanQueryPool::Destroy()
{
    if (m_vkQueryPool)
    {
        RemoveDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_QUERY_POOL, (uint64_t)m_vkQueryPool);
        m_pGlobal->vk.vkDestroyQueryPool(m_pGlobal->device.GetVkDevice(), m_vkQueryPool, nullptr);
        m_vkQueryPool = VK_NULL_HANDLE;
    }
}

}
