/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "CommandBuffers.h"

#include "../Global.h"
#include "../Usages.h"

#include "../Vulkan/VulkanCommandPool.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::GPU
{

CommandBuffers::CommandBuffers(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

CommandBuffers::~CommandBuffers()
{
    m_pGlobal = nullptr;
}

void CommandBuffers::Destroy()
{
    m_pGlobal->pLogger->Info("CommandBuffers: Destroying");

    while (!m_commandBuffers.empty())
    {
        DestroyCommandBuffer(m_commandBuffers.cbegin()->first);
    }
}

std::expected<CommandBuffer*, bool> CommandBuffers::AcquireCommandBuffer(VulkanCommandPool* pCommandPool,
                                                                         CommandBufferType type,
                                                                         const std::string& tag)
{
    auto commandBuffer = CommandBuffer::Create(m_pGlobal, pCommandPool, type, tag);
    if (!commandBuffer)
    {
        m_pGlobal->pLogger->Error("CommandBuffers::AcquireCommandBuffer: Failed to create command buffer");
        return std::unexpected(false);
    }

    auto commandBufferDynamic = std::make_unique<CommandBuffer>(*commandBuffer);
    auto pCommandBuffer = commandBufferDynamic.get();

    //
    // Record the command buffer internally
    //
    std::lock_guard<std::mutex> lock(m_commandBuffersMutex);
    m_commandBuffers.insert({pCommandBuffer->GetId(), std::move(commandBufferDynamic)});

    return pCommandBuffer;
}

std::optional<CommandBuffer*> CommandBuffers::GetCommandBuffer(CommandBufferId commandBufferId) const
{
    std::lock_guard<std::mutex> lock(m_commandBuffersMutex);

    const auto it = m_commandBuffers.find(commandBufferId);
    if (it == m_commandBuffers.cend())
    {
        return std::nullopt;
    }

    return it->second.get();
}

void CommandBuffers::DestroyCommandBuffer(CommandBufferId commandBufferId)
{
    auto commandBuffer = GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        return;
    }

    m_pGlobal->pLogger->Debug("CommandBuffers: Destroying command buffer: {}", commandBufferId.id);

    (*commandBuffer)->Destroy();

    std::lock_guard<std::mutex> lock(m_commandBuffersMutex);
    m_commandBuffers.erase(commandBufferId);
}

void CommandBuffers::RunCleanUp()
{
    std::lock_guard<std::mutex> lock(m_commandBuffersMutex);

    //
    // Destroy / clean up any command buffers that are no longer
    // used and whose work has finished executing
    //
    std::unordered_set<CommandBufferId> cleanedUpCommandBufferIds;

    for (const auto& it : m_commandBuffers)
    {
        // If something is still using it (e.g. a Frame which has it listed
        // as an associated command buffer), don't clean it up
        if (m_pGlobal->pUsages->commandBuffers.GetGPUUsageCount(it.first) > 0)
        {
            continue;
        }

        // If its work hasn't finished executing, don't clean it up
        const auto fenceStatus = m_pGlobal->vk.vkGetFenceStatus(m_pGlobal->device.GetVkDevice(), it.second->GetVkFence());
        if (fenceStatus == VK_NOT_READY)
        {
            continue;
        }

        // Mark the command buffer as no longer referencing its resources
        it.second->ReleaseTrackedResources();

        // Destroy the command buffer
        it.second->Destroy();

        cleanedUpCommandBufferIds.insert(it.first);
    }

    for (const auto& commandBufferId : cleanedUpCommandBufferIds)
    {
        m_commandBuffers.erase(commandBufferId);
    }
}

}
