/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanCommandPool.h"
#include "VulkanDebugUtil.h"

#include "../Global.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::GPU
{

std::expected<VulkanCommandPool, bool> VulkanCommandPool::Create(Global* pGlobal,
                                                                 const uint32_t& queueFamilyIndex,
                                                                 const VkCommandPoolCreateFlags& vkCreateFlags,
                                                                 const std::string& tag)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = vkCreateFlags;

    VkCommandPool vkCommandPool{VK_NULL_HANDLE};

    if (auto result = pGlobal->vk.vkCreateCommandPool(pGlobal->device.GetVkDevice(), &poolInfo, nullptr, &vkCommandPool); result != VK_SUCCESS)
    {
        pGlobal->pLogger->Error("VulkanCommandPool::Create: vkCreateCommandPool call failure, result code: {}", (uint32_t)result);
        return std::unexpected(false);
    }

    SetDebugName(pGlobal->vk, pGlobal->device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)vkCommandPool, std::format("CommandPool-{}", tag));

    return VulkanCommandPool(pGlobal, vkCommandPool, vkCreateFlags);
}

VulkanCommandPool::VulkanCommandPool(Global* pGlobal, VkCommandPool vkCommandPool, VkCommandPoolCreateFlags vkCreateFlags)
    : m_pGlobal(pGlobal)
    , m_vkCommandPool(vkCommandPool)
    , m_vkCreateFlags(vkCreateFlags)
{

}

VulkanCommandPool::~VulkanCommandPool()
{
    m_pGlobal = nullptr;
    m_vkCommandPool = VK_NULL_HANDLE;
    m_vkCreateFlags = {};
}

void VulkanCommandPool::Destroy()
{
    FreeAllCommandBuffers();

    if (m_vkCommandPool != VK_NULL_HANDLE)
    {
        RemoveDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)m_vkCommandPool);
        m_pGlobal->vk.vkDestroyCommandPool(m_pGlobal->device.GetVkDevice(), m_vkCommandPool, nullptr);

        m_vkCommandPool = VK_NULL_HANDLE;
        m_vkCreateFlags = {};
    }
}

std::expected<VulkanCommandBuffer, bool> VulkanCommandPool::AllocateCommandBuffer(CommandBufferType type, const std::string& tag)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_vkCommandPool;
    allocInfo.commandBufferCount = 1;

    switch (type)
    {
        case CommandBufferType::Primary: allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; break;
        case CommandBufferType::Secondary: allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY; break;
    }

    VkCommandBuffer vkCommandBuffer{VK_NULL_HANDLE};

    const auto result = m_pGlobal->vk.vkAllocateCommandBuffers(m_pGlobal->device.GetVkDevice(), &allocInfo, &vkCommandBuffer);
    if (result != VK_SUCCESS)
    {
        m_pGlobal->pLogger->Error("VulkanCommandPool::AllocateCommandBuffer: vkAllocateCommandBuffers call failure, result code: {}", (uint32_t)result);
        return std::unexpected(false);
    }

    SetDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)vkCommandBuffer, std::format("CommandBuffer-{}", tag));

    auto vulkanCommandBuffer = VulkanCommandBuffer(m_pGlobal, type, vkCommandBuffer, tag);

    m_allocatedCommandBuffers.insert(vulkanCommandBuffer);

    return vulkanCommandBuffer;
}

void VulkanCommandPool::FreeCommandBuffer(const VulkanCommandBuffer& commandBuffer)
{
    const auto it = m_allocatedCommandBuffers.find(commandBuffer);
    if (it != m_allocatedCommandBuffers.cend())
    {
        const auto vkCommandBuffer = commandBuffer.GetVkCommandBuffer();

        RemoveDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)vkCommandBuffer);
        m_pGlobal->vk.vkFreeCommandBuffers(m_pGlobal->device.GetVkDevice(), m_vkCommandPool, 1, &vkCommandBuffer);
        m_allocatedCommandBuffers.erase(it);
    }
}

void VulkanCommandPool::FreeAllCommandBuffers()
{
    while (!m_allocatedCommandBuffers.empty())
    {
        FreeCommandBuffer(*m_allocatedCommandBuffers.cbegin());
    }
}

void VulkanCommandPool::ResetCommandBuffer(const VulkanCommandBuffer& commandBuffer, bool trimMemory)
{
    if (!(m_vkCreateFlags & VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT))
    {
        m_pGlobal->pLogger->Error(
            "VulkanCommandPool::ResetCommandBuffer: Attempted to reset command buffer in a pool that doesn't support individual resetting");
        return;
    }

    VkCommandBufferResetFlags flags = 0;
    if (trimMemory)
    {
        flags = VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT;
    }

    const auto it = m_allocatedCommandBuffers.find(commandBuffer);
    if (it != m_allocatedCommandBuffers.cend())
    {
        const auto vkCommandBuffer = commandBuffer.GetVkCommandBuffer();
        m_pGlobal->vk.vkResetCommandBuffer(vkCommandBuffer, flags);
    }
}

void VulkanCommandPool::ResetPool(bool trimMemory)
{
    VkCommandPoolResetFlags flags = 0;
    if (trimMemory)
    {
        flags = VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT;
    }

    m_pGlobal->vk.vkResetCommandPool(m_pGlobal->device.GetVkDevice(), m_vkCommandPool, flags);
}

}
