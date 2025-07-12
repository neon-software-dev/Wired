/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanQueue.h"
#include "VulkanDebugUtil.h"

#include "../Global.h"

#include <NEON/Common/Log/ILogger.h>

#include <algorithm>

namespace Wired::GPU
{

VulkanQueue VulkanQueue::CreateFrom(Global* pGlobal, VkQueue vkQueue, uint32_t queueFamilyIndex, const std::string& tag)
{
    SetDebugName(pGlobal->vk, pGlobal->device, VK_OBJECT_TYPE_QUEUE, (uint64_t)vkQueue, std::format("Queue-{}", tag));

    return {pGlobal, vkQueue, queueFamilyIndex, tag};
}

VulkanQueue::VulkanQueue(Global* pGlobal, VkQueue vkQueue,  uint32_t queueFamilyIndex, std::string tag)
    : m_pGlobal(pGlobal)
    , m_vkQueue(vkQueue)
    , m_queueFamilyIndex(queueFamilyIndex)
    , m_tag(std::move(tag))
{

}

VulkanQueue::~VulkanQueue()
{
    m_pGlobal = nullptr;
    m_vkQueue = VK_NULL_HANDLE;
    m_queueFamilyIndex = 0;
    m_tag = {};
}

void VulkanQueue::Destroy()
{
    if (m_vkQueue != VK_NULL_HANDLE)
    {
        RemoveDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_QUEUE, (uint64_t) m_vkQueue);
    }
}

bool VulkanQueue::SubmitBatch(const std::vector<VulkanCommandBuffer>& commandBuffers,
                              const WaitOn& waitOn,
                              const SignalOn& signalOn,
                              const std::optional<VkFence>& vkFence,
                              const std::string& submitTag)
{
    QueueSectionLabel submitSection(m_pGlobal, m_vkQueue, std::format("Submit-{}", submitTag));

    std::vector<VkSemaphoreSubmitInfo> semaphoreWaits;
    std::ranges::transform(waitOn.semaphores, std::back_inserter(semaphoreWaits), [](const SemaphoreOp& semaphoreOp){
        VkSemaphoreSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        submitInfo.semaphore = semaphoreOp.semaphore;
        submitInfo.stageMask = semaphoreOp.stageMask;
        return submitInfo;
    });

    std::vector<VkSemaphoreSubmitInfo> semaphoreSignals;
    std::ranges::transform(signalOn.semaphores, std::back_inserter(semaphoreSignals), [](const SemaphoreOp& semaphoreOp){
        VkSemaphoreSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        submitInfo.semaphore = semaphoreOp.semaphore;
        submitInfo.stageMask = semaphoreOp.stageMask;
        return submitInfo;
    });

    std::vector<VkCommandBufferSubmitInfo> bufferSubmits;
    std::ranges::transform(commandBuffers, std::back_inserter(bufferSubmits), [](const auto& commandBuffer){
        VkCommandBufferSubmitInfo bufferSubmit{};
        bufferSubmit.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        bufferSubmit.commandBuffer = commandBuffer.GetVkCommandBuffer();
        return bufferSubmit;
    });

    VkSubmitInfo2 submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.waitSemaphoreInfoCount = (uint32_t)semaphoreWaits.size();
    submitInfo.pWaitSemaphoreInfos = semaphoreWaits.data();
    submitInfo.signalSemaphoreInfoCount = (uint32_t)semaphoreSignals.size();
    submitInfo.pSignalSemaphoreInfos = semaphoreSignals.data();
    submitInfo.commandBufferInfoCount = (uint32_t)bufferSubmits.size();
    submitInfo.pCommandBufferInfos = bufferSubmits.data();

    const auto result = m_pGlobal->vk.vkQueueSubmit2(m_vkQueue, 1U, &submitInfo, vkFence ? *vkFence : VK_NULL_HANDLE);
    if (result != VK_SUCCESS)
    {
        m_pGlobal->pLogger->Error("VulkanQueue::Submit: Failed to submit command buffer(s) to queue: {}, for submit: {}", m_tag, submitTag);
        return false;
    }

    return true;
}

}
