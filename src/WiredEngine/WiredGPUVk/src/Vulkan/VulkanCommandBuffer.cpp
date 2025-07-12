/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanCommandBuffer.h"
#include "VulkanDebugUtil.h"

#include "../Global.h"

#include <NEON/Common/Log/ILogger.h>

#include <cassert>
#include <algorithm>

namespace Wired::GPU
{

VulkanCommandBuffer::VulkanCommandBuffer(Global* pGlobal, CommandBufferType commandBufferType, VkCommandBuffer vkCommandBuffer, std::string tag)
    : m_pGlobal(pGlobal)
    , m_commandBufferType(commandBufferType)
    , m_vkCommandBuffer(vkCommandBuffer)
    , m_tag(std::move(tag))
{

}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    m_pGlobal = nullptr;
    m_commandBufferType = {};
    m_vkCommandBuffer = VK_NULL_HANDLE;
    m_tag = {};
}

void VulkanCommandBuffer::Begin(const VkCommandBufferUsageFlagBits& flags) const
{
    assert(m_vkCommandBuffer != VK_NULL_HANDLE);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;

    VkCommandBufferInheritanceInfo inheritanceInfo{};
    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

    VkCommandBufferInheritanceRenderingInfo inheritanceRenderingInfo{};
    inheritanceRenderingInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;

    if (m_commandBufferType == CommandBufferType::Secondary)
    {
        inheritanceInfo.pNext = &inheritanceRenderingInfo;
        beginInfo.pInheritanceInfo = &inheritanceInfo;
    }

    auto result = m_pGlobal->vk.vkBeginCommandBuffer(m_vkCommandBuffer, &beginInfo);
    if (result != VK_SUCCESS)
    {
        m_pGlobal->pLogger->Error("VulkanCommandBuffer::Begin: vkBeginCommandBuffer call failure, error code: {}", (uint32_t)result);
    }
}

void VulkanCommandBuffer::End() const
{
    assert(m_vkCommandBuffer != VK_NULL_HANDLE);

    const auto result = m_pGlobal->vk.vkEndCommandBuffer(m_vkCommandBuffer);
    if (result != VK_SUCCESS)
    {
        m_pGlobal->pLogger->Error("VulkanCommandBuffer::End: vkEndCommandBuffer call failure, error code: {}", (uint32_t)result);
    }
}

bool VulkanCommandBuffer::operator==(const VulkanCommandBuffer& other) const
{
    return m_vkCommandBuffer == other.m_vkCommandBuffer;
}

void VulkanCommandBuffer::CmdPipelineBarrier2(const Barrier& barrier) const
{
    std::vector<VkImageMemoryBarrier2> imageBarriers;

    std::ranges::transform(barrier.imageBarriers, std::back_inserter(imageBarriers), [](const auto& imageBarrier){
        VkImageMemoryBarrier2 vkImageBarrier{};
        vkImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        vkImageBarrier.srcStageMask = imageBarrier.srcStageMask;
        vkImageBarrier.srcAccessMask = imageBarrier.srcAccessMask;
        vkImageBarrier.dstStageMask = imageBarrier.dstStageMask;
        vkImageBarrier.dstAccessMask = imageBarrier.dstAccessMask;
        vkImageBarrier.oldLayout = imageBarrier.oldLayout;
        vkImageBarrier.newLayout = imageBarrier.newLayout;
        vkImageBarrier.srcQueueFamilyIndex = imageBarrier.srcQueueFamilyIndex;
        vkImageBarrier.dstQueueFamilyIndex = imageBarrier.dstQueueFamilyIndex;
        vkImageBarrier.image = imageBarrier.vkImage;
        vkImageBarrier.subresourceRange = imageBarrier.subresourceRange;

        return vkImageBarrier;
    });

    std::vector<VkBufferMemoryBarrier2> bufferBarriers;

    std::ranges::transform(barrier.bufferBarriers, std::back_inserter(bufferBarriers), [](const auto& bufferBarrier){
        VkBufferMemoryBarrier2 vkBufferBarrier{};
        vkBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        vkBufferBarrier.srcStageMask = bufferBarrier.srcStageMask;
        vkBufferBarrier.srcAccessMask = bufferBarrier.srcAccessMask;
        vkBufferBarrier.dstStageMask = bufferBarrier.dstStageMask;
        vkBufferBarrier.dstAccessMask = bufferBarrier.dstAccessMask;
        vkBufferBarrier.srcQueueFamilyIndex = bufferBarrier.srcQueueFamilyIndex;
        vkBufferBarrier.dstQueueFamilyIndex = bufferBarrier.dstQueueFamilyIndex;
        vkBufferBarrier.buffer = bufferBarrier.vkBuffer;
        vkBufferBarrier.offset = bufferBarrier.byteOffset;
        vkBufferBarrier.size = bufferBarrier.byteSize;

        return vkBufferBarrier;
    });

    VkDependencyInfo vkDependencyInfo{};
    vkDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    vkDependencyInfo.dependencyFlags = 0;
    vkDependencyInfo.imageMemoryBarrierCount = (uint32_t)imageBarriers.size();
    vkDependencyInfo.pImageMemoryBarriers = imageBarriers.data();
    vkDependencyInfo.bufferMemoryBarrierCount = (uint32_t)bufferBarriers.size();
    vkDependencyInfo.pBufferMemoryBarriers = bufferBarriers.data();

    m_pGlobal->vk.vkCmdPipelineBarrier2(m_vkCommandBuffer, &vkDependencyInfo);
}

void VulkanCommandBuffer::CmdClearColorImage(VkImage vkImage, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges) const
{
    m_pGlobal->vk.vkCmdClearColorImage(m_vkCommandBuffer, vkImage, imageLayout, pColor, rangeCount, pRanges);
}

void VulkanCommandBuffer::CmdBlitImage(VkImage vkSourceImage, VkImageLayout vkSourceImageLayout, VkImage vkDestImage, VkImageLayout vkDestImageLayout, VkImageBlit vkImageBlit, VkFilter vkFilter) const
{
    m_pGlobal->vk.vkCmdBlitImage(m_vkCommandBuffer, vkSourceImage, vkSourceImageLayout, vkDestImage, vkDestImageLayout, 1, &vkImageBlit, vkFilter);
}

void VulkanCommandBuffer::CmdExecuteCommands(const std::vector<VulkanCommandBuffer>& commands) const
{
    CmdBufferSectionLabel section(m_pGlobal, m_vkCommandBuffer, std::format("CmdExecute-{}", m_tag));

    std::vector<VkCommandBuffer> vkCommandBuffers;
    vkCommandBuffers.reserve(commands.size());

    std::ranges::transform(commands, std::back_inserter(vkCommandBuffers), [](const auto& commandBuffer){
        return commandBuffer.GetVkCommandBuffer();
    });

    m_pGlobal->vk.vkCmdExecuteCommands(m_vkCommandBuffer, (uint32_t)vkCommandBuffers.size(), vkCommandBuffers.data());
}

void VulkanCommandBuffer::CmdCopyBuffer2(const VkCopyBufferInfo2* pCopyBufferInfo) const
{
    m_pGlobal->vk.vkCmdCopyBuffer2(m_vkCommandBuffer, pCopyBufferInfo);
}

void VulkanCommandBuffer::CmdCopyBufferToImage2(const VkCopyBufferToImageInfo2* pCopyBufferToImageInfo) const
{
    m_pGlobal->vk.vkCmdCopyBufferToImage2(m_vkCommandBuffer, pCopyBufferToImageInfo);
}

void VulkanCommandBuffer::CmdBeginRendering(const VkRenderingInfo& vkRenderingInfo) const
{
    m_pGlobal->vk.vkCmdBeginRendering(m_vkCommandBuffer, &vkRenderingInfo);
}

void VulkanCommandBuffer::CmdEndRendering()
{
    m_pGlobal->vk.vkCmdEndRendering(m_vkCommandBuffer);
}

void VulkanCommandBuffer::CmdBindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) const
{
    m_pGlobal->vk.vkCmdBindPipeline(m_vkCommandBuffer, pipelineBindPoint, pipeline);
}

void VulkanCommandBuffer::CmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets) const
{
    m_pGlobal->vk.vkCmdBindVertexBuffers(m_vkCommandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

void VulkanCommandBuffer::CmdBindIndexBuffer(VkBuffer vkBuffer, const std::size_t& byteOffset, VkIndexType vkIndexType) const
{
    m_pGlobal->vk.vkCmdBindIndexBuffer(m_vkCommandBuffer, vkBuffer, byteOffset, vkIndexType);
}

void VulkanCommandBuffer::CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const
{
    m_pGlobal->vk.vkCmdDrawIndexed(m_vkCommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandBuffer::CmdDrawIndexedIndirect(VkBuffer vkBuffer, const std::size_t& byteOffset, uint32_t drawCount, uint32_t stride) const
{
    m_pGlobal->vk.vkCmdDrawIndexedIndirect(m_vkCommandBuffer, vkBuffer, byteOffset, drawCount, stride);
}

void VulkanCommandBuffer::CmdDrawIndexedIndirectCount(VkBuffer vkCommandsBuffer, const std::size_t& commandsByteOffset, VkBuffer vkCountsBuffer, const std::size_t& countsByteOffset, uint32_t maxDrawCount, uint32_t stride) const
{
    m_pGlobal->vk.vkCmdDrawIndexedIndirectCount(m_vkCommandBuffer, vkCommandsBuffer, commandsByteOffset, vkCountsBuffer, countsByteOffset, maxDrawCount, stride);
}

void VulkanCommandBuffer::CmdBindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets) const
{
    m_pGlobal->vk.vkCmdBindDescriptorSets(m_vkCommandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

void VulkanCommandBuffer::CmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    m_pGlobal->vk.vkCmdDispatch(m_vkCommandBuffer, groupCountX, groupCountY, groupCountZ);
}

void VulkanCommandBuffer::CmdSetDepthTestEnable(bool enable)
{
    m_pGlobal->vk.vkCmdSetDepthTestEnable(m_vkCommandBuffer, enable);
}

void VulkanCommandBuffer::CmdSetDepthWriteEnable(bool enable)
{
    m_pGlobal->vk.vkCmdSetDepthWriteEnable(m_vkCommandBuffer, enable);
}

}
