/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "CommandBuffer.h"

#include "../Global.h"
#include "../Usages.h"
#include "../Buffer/Buffers.h"

#include "../Vulkan/VulkanDebugUtil.h"

#include <NEON/Common/Log/ILogger.h>

#include <cassert>

namespace Wired::GPU
{

struct ImageBarrierFlags
{
    VkPipelineStageFlags2 stageMask{};
    VkAccessFlags2 accessMask{};
    VkImageLayout layout{};
};

ImageBarrierFlags GetSourceImageUsageBarrierFlags(ImageUsageMode usageMode)
{
    ImageBarrierFlags flags{};

    switch (usageMode)
    {
        case ImageUsageMode::Undefined:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_NONE_KHR;
            flags.accessMask = VK_ACCESS_2_NONE_KHR;
            flags.layout = VK_IMAGE_LAYOUT_UNDEFINED;
        }
        break;
        case ImageUsageMode::GraphicsSampled:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            flags.accessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
            flags.layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
        }
        break;
        case ImageUsageMode::ComputeSampled:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            flags.accessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
            flags.layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
        }
        break;
        case ImageUsageMode::TransferSrc:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            flags.accessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            flags.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        }
        break;
        case ImageUsageMode::TransferDst:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            flags.accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            flags.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        }
        break;
        case ImageUsageMode::ColorAttachment:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            flags.accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            flags.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        }
        break;
        case ImageUsageMode::DepthAttachment:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            flags.accessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            flags.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        }
        break;
        case ImageUsageMode::PresentSrc:
        {
            /* no-op */
        }
        break;
        case ImageUsageMode::GraphicsStorageRead:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            flags.accessMask = VK_ACCESS_2_SHADER_READ_BIT;
            flags.layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
        }
        break;
        case ImageUsageMode::ComputeStorageRead:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            flags.accessMask = VK_ACCESS_2_SHADER_READ_BIT;
            flags.layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
        }
        break;
        case ImageUsageMode::ComputeStorageReadWrite:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            flags.accessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
            flags.layout = VK_IMAGE_LAYOUT_GENERAL;
        }
        break;
    }

    return flags;
}

ImageBarrierFlags GetDestImageUsageBarrierFlags(ImageUsageMode usageMode)
{
    ImageBarrierFlags flags{};

    switch (usageMode)
    {
        case ImageUsageMode::Undefined:
        {
            /* no-op */
        }
        break;
        case ImageUsageMode::GraphicsSampled:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            flags.accessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
            flags.layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
        }
        break;
        case ImageUsageMode::ComputeSampled:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            flags.accessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
            flags.layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
        }
        break;
        case ImageUsageMode::TransferSrc:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            flags.accessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            flags.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        }
        break;
        case ImageUsageMode::TransferDst:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            flags.accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            flags.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        }
        break;
        case ImageUsageMode::ColorAttachment:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            flags.accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            flags.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        }
        break;
        case ImageUsageMode::DepthAttachment:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            flags.accessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            flags.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        }
        break;
        case ImageUsageMode::PresentSrc:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            flags.accessMask = VK_ACCESS_2_NONE;
            flags.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
        break;
        case ImageUsageMode::GraphicsStorageRead:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            flags.accessMask = VK_ACCESS_2_SHADER_READ_BIT;
            flags.layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
        }
        break;
        case ImageUsageMode::ComputeStorageRead:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            flags.accessMask = VK_ACCESS_2_SHADER_READ_BIT;
            flags.layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
        }
        break;
        case ImageUsageMode::ComputeStorageReadWrite:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            flags.accessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
            flags.layout = VK_IMAGE_LAYOUT_GENERAL;
        }
        break;
    }

    return flags;
}

struct BufferBarrierFlags
{
    VkPipelineStageFlags2 stageMask{};
    VkAccessFlags2 accessMask{};
};

BufferBarrierFlags GetSourceBufferUsageBarrierFlags(BufferUsageMode usageMode)
{
    BufferBarrierFlags flags{};

    switch (usageMode)
    {
        case BufferUsageMode::TransferSrc:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            flags.accessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        }
        break;
        case BufferUsageMode::TransferDst:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            flags.accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        }
        break;
        case BufferUsageMode::VertexRead:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
            flags.accessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
        }
        break;
        case BufferUsageMode::IndexRead:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
            flags.accessMask = VK_ACCESS_2_INDEX_READ_BIT;
        }
        break;
        case BufferUsageMode::Indirect:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
            flags.accessMask = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
        }
        break;
        case BufferUsageMode::GraphicsUniformRead:
        case BufferUsageMode::GraphicsStorageRead:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            flags.accessMask = VK_ACCESS_2_SHADER_READ_BIT;
        }
        break;
        case BufferUsageMode::ComputeUniformRead:
        case BufferUsageMode::ComputeStorageRead:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            flags.accessMask = VK_ACCESS_2_SHADER_READ_BIT;
        }
        break;
        case BufferUsageMode::ComputeStorageReadWrite:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            flags.accessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
        }
        break;
    }

    return flags;
}

BufferBarrierFlags GetDestBufferUsageBarrierFlags(BufferUsageMode usageMode)
{
    BufferBarrierFlags flags{};

    switch (usageMode)
    {
        case BufferUsageMode::TransferSrc:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            flags.accessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        }
        break;
        case BufferUsageMode::TransferDst:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            flags.accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        }
        break;
        case BufferUsageMode::VertexRead:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
            flags.accessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
        }
        break;
        case BufferUsageMode::IndexRead:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
            flags.accessMask = VK_ACCESS_2_INDEX_READ_BIT;
        }
        break;
        case BufferUsageMode::Indirect:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
            flags.accessMask = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
        }
        break;
        case BufferUsageMode::GraphicsUniformRead:
        case BufferUsageMode::GraphicsStorageRead:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            flags.accessMask = VK_ACCESS_2_SHADER_READ_BIT;
        }
        break;
        case BufferUsageMode::ComputeUniformRead:
        case BufferUsageMode::ComputeStorageRead:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            flags.accessMask = VK_ACCESS_2_SHADER_READ_BIT;
        }
        break;
        case BufferUsageMode::ComputeStorageReadWrite:
        {
            flags.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            flags.accessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
        }
        break;
    }

    return flags;
}

std::expected<CommandBuffer, bool> CommandBuffer::Create(Global* pGlobal,
                                                         VulkanCommandPool* pVulkanCommandPool,
                                                         CommandBufferType type,
                                                         const std::string& tag)
{
    //
    // Allocate a vulkan command buffer
    //
    const auto vulkanCommandBuffer = pVulkanCommandPool->AllocateCommandBuffer(type, tag);
    if (!vulkanCommandBuffer)
    {
        pGlobal->pLogger->Error("CommandBufferState::Create: Allocating command buffer failed");
        return std::unexpected(false);
    }

    //
    // Create a fence to track the command buffer's execution
    //
    VkFenceCreateInfo vkFenceCreateInfo{};
    vkFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence vkFence{VK_NULL_HANDLE};

    if (type == CommandBufferType::Primary)
    {
        const auto result = pGlobal->vk.vkCreateFence(pGlobal->device.GetVkDevice(), &vkFenceCreateInfo, nullptr, &vkFence);
        if (result != VK_SUCCESS)
        {
            pGlobal->pLogger->Error("CommandBufferState::Create: vkCreateFence() call failed, error code: {}", (uint32_t) result);
            return std::unexpected(false);
        }
        SetDebugName(pGlobal->vk, pGlobal->device, VK_OBJECT_TYPE_FENCE, (uint64_t) vkFence, std::format("Fence-{}-Finished", tag));
    }

    //
    // Obtain an id and return the created command buffer
    //
    const auto commandBufferId = pGlobal->ids.commandBufferIds.GetId();

    return CommandBuffer(pGlobal, tag, type, commandBufferId, pVulkanCommandPool, *vulkanCommandBuffer, vkFence);
}

CommandBuffer::CommandBuffer(Global* pGlobal,
                             std::string tag,
                             CommandBufferType type,
                             CommandBufferId commandBufferId,
                             VulkanCommandPool* pVulkanCommandPool,
                             const VulkanCommandBuffer& vulkanCommandBuffer,
                             VkFence vkFence)
    : m_pGlobal(pGlobal)
    , m_tag(std::move(tag))
    , m_type(type)
    , m_id(commandBufferId)
    , m_pVulkanCommandPool(pVulkanCommandPool)
    , m_vulkanCommandBuffer(vulkanCommandBuffer)
    , m_vkFence(vkFence)
{

}

CommandBuffer::~CommandBuffer()
{
    m_pGlobal = nullptr;
    m_tag = {};
    m_id = {};
    m_pVulkanCommandPool = nullptr;
    m_vulkanCommandBuffer = {};
    m_vkFence = VK_NULL_HANDLE;
}

void CommandBuffer::Destroy()
{
    if (m_vkFence != VK_NULL_HANDLE)
    {
        RemoveDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_FENCE, (uint64_t)m_vkFence);
        m_pGlobal->vk.vkDestroyFence(m_pGlobal->device.GetVkDevice(), m_vkFence, nullptr);
        m_vkFence = VK_NULL_HANDLE;
    }

    if (m_vulkanCommandBuffer.IsValid())
    {
        m_pVulkanCommandPool->FreeCommandBuffer(m_vulkanCommandBuffer);
    }

    if (m_id.IsValid())
    {
        m_pGlobal->ids.commandBufferIds.ReturnId(m_id);
        m_id = {};
    }
}

void CommandBuffer::ConfigureForPresentation(SemaphoreOp waitOn, SemaphoreOp signalOn)
{
    assert(m_type == CommandBufferType::Primary);

    m_waitSemaphores.push_back(waitOn);
    m_signalSemaphores.push_back(signalOn);

    m_configuredForPresent = true;
}

bool CommandBuffer::IsInAnyPass()
{
    return m_state != CommandBufferState::Default;
}

bool CommandBuffer::BeginCopyPass()
{
    if (m_state != CommandBufferState::Default)
    {
        m_pGlobal->pLogger->Error("CommandBuffer::BeginCopyPass: Can't begin copy pass within another pass");
        return false;
    }

    m_state = CommandBufferState::CopyPass;

    return true;
}

bool CommandBuffer::EndCopyPass()
{
    if (m_state != CommandBufferState::CopyPass)
    {
        m_pGlobal->pLogger->Error("CommandBuffer::EndCopyPass: Not in a copy pass!");
        return false;
    }

    m_state = CommandBufferState::Default;

    return true;
}

bool CommandBuffer::IsInCopyPass()
{
    return m_state == CommandBufferState::CopyPass;
}

bool CommandBuffer::BeginRenderPass()
{
    if (m_state != CommandBufferState::Default)
    {
        m_pGlobal->pLogger->Error("CommandBuffer::BeginRenderPass: Can't begin copy pass within another pass");
        return false;
    }

    m_state = CommandBufferState::RenderPass;

    return true;
}

bool CommandBuffer::EndRenderPass()
{
    if (m_state != CommandBufferState::RenderPass)
    {
        m_pGlobal->pLogger->Error("CommandBuffer::EndRenderPass: Not in a render pass!");
        return false;
    }

    m_state = CommandBufferState::Default;

    return true;
}

bool CommandBuffer::IsInRenderPass()
{
    return m_state == CommandBufferState::RenderPass;
}

bool CommandBuffer::BeginComputePass()
{
    if (m_state != CommandBufferState::Default)
    {
        m_pGlobal->pLogger->Error("CommandBuffer::BeginComputePass: Can't begin compute pass within another pass");
        return false;
    }

    m_state = CommandBufferState::ComputePass;
    m_passState = PassState{};

    return true;
}

bool CommandBuffer::EndComputePass()
{
    if (m_state != CommandBufferState::ComputePass)
    {
        m_pGlobal->pLogger->Error("CommandBuffer::EndComputePass: Not in a compute pass!");
        return false;
    }

    m_state = CommandBufferState::Default;
    m_passState = std::nullopt;

    return true;
}

bool CommandBuffer::IsInComputePass()
{
    return m_state == CommandBufferState::ComputePass;
}

void CommandBuffer::CmdImagePipelineBarrier(const GPUImage& loadedImage,
                                            VkImageSubresourceRange vkImageSubresourceRange,
                                            ImageUsageMode sourceUsageMode,
                                            ImageUsageMode destUsageMode)
{
    if (sourceUsageMode == destUsageMode) { return; }

    const auto sourceFlags = GetSourceImageUsageBarrierFlags(sourceUsageMode);
    const auto destFlags = GetDestImageUsageBarrierFlags(destUsageMode);

    m_vulkanCommandBuffer.CmdPipelineBarrier2(Barrier{
        .imageBarriers = {
            ImageBarrier{
                .vkImage = loadedImage.imageData.vkImage,
                .subresourceRange = vkImageSubresourceRange,
                .srcStageMask = sourceFlags.stageMask,
                .srcAccessMask = sourceFlags.accessMask,
                .dstStageMask = destFlags.stageMask,
                .dstAccessMask = destFlags.accessMask,
                .oldLayout = sourceFlags.layout,
                .newLayout = destFlags.layout
            }
        },
        .bufferBarriers = {}
    });

    // Record usages
    RecordImageUsage(loadedImage.imageData.vkImage);
}

void CommandBuffer::CmdBufferPipelineBarrier(const GPUBuffer& gpuBuffer,
                                             const std::size_t& byteOffset,
                                             const std::size_t& byteSize,
                                             BufferUsageMode sourceUsageMode,
                                             BufferUsageMode destUsageMode)
{
    if (sourceUsageMode == destUsageMode) { return; }

    const auto sourceFlags = GetSourceBufferUsageBarrierFlags(sourceUsageMode);
    const auto destFlags = GetDestBufferUsageBarrierFlags(destUsageMode);

    m_vulkanCommandBuffer.CmdPipelineBarrier2(Barrier{
        .imageBarriers = {},
        .bufferBarriers = {
            BufferBarrier{
                .vkBuffer = gpuBuffer.vkBuffer,
                .byteOffset = byteOffset,
                .byteSize = byteSize,
                .srcStageMask = sourceFlags.stageMask,
                .srcAccessMask = sourceFlags.accessMask,
                .dstStageMask = destFlags.stageMask,
                .dstAccessMask = destFlags.accessMask
            }
        }
    });

    // Record usages
    RecordBufferUsage(gpuBuffer.vkBuffer);
}

void CommandBuffer::CmdClearColorImage(const GPUImage& loadedImage,
                                       VkImageLayout imageLayout,
                                       const VkClearColorValue* pColor,
                                       uint32_t rangeCount,
                                       const VkImageSubresourceRange* pRanges)
{
    m_vulkanCommandBuffer.CmdClearColorImage(loadedImage.imageData.vkImage, imageLayout, pColor, rangeCount, pRanges);

    // Record usages
    RecordImageUsage(loadedImage.imageData.vkImage);
}

void CommandBuffer::CmdBlitImage(const GPUImage& sourceImage,
                                 VkImageLayout sourceImageLayout,
                                 const GPUImage& destImage,
                                 VkImageLayout destImageLayout,
                                 VkImageBlit vkImageBlit,
                                 VkFilter vkFilter)
{
    m_vulkanCommandBuffer.CmdBlitImage(sourceImage.imageData.vkImage, sourceImageLayout, destImage.imageData.vkImage, destImageLayout, vkImageBlit, vkFilter);

    // Record usages
    RecordImageUsage(sourceImage.imageData.vkImage);
    RecordImageUsage(destImage.imageData.vkImage);
}

void CommandBuffer::CmdExecuteCommands(const std::vector<CommandBuffer*>& secondaryCommandBuffers)
{
    std::vector<VulkanCommandBuffer> vulkanCommandBuffers;

    std::ranges::transform(secondaryCommandBuffers, std::back_inserter(vulkanCommandBuffers), [](const auto& commandBuffer){
       return commandBuffer->GetVulkanCommandBuffer();
    });

    m_vulkanCommandBuffer.CmdExecuteCommands(vulkanCommandBuffers);

    for (const auto& commandBuffer : secondaryCommandBuffers)
    {
        m_secondaryCommandBuffers.insert(commandBuffer->GetId());
    }
}

void CommandBuffer::CmdCopyBuffer2(const VkCopyBufferInfo2* pCopyBufferInfo)
{
    m_vulkanCommandBuffer.CmdCopyBuffer2(pCopyBufferInfo);

    // Record usages
    RecordBufferUsage(pCopyBufferInfo->srcBuffer);
    RecordBufferUsage(pCopyBufferInfo->dstBuffer);
}

void CommandBuffer::CmdCopyBufferToImage2(const VkCopyBufferToImageInfo2* pCopyBufferToImageInfo)
{
    m_vulkanCommandBuffer.CmdCopyBufferToImage2(pCopyBufferToImageInfo);

    // Record usages
    RecordBufferUsage(pCopyBufferToImageInfo->srcBuffer);
    RecordImageUsage(pCopyBufferToImageInfo->dstImage);
}

void CommandBuffer::CmdBeginRendering(const VkRenderingInfo& vkRenderingInfo,
                                      const std::vector<RenderPassAttachment>& colorAttachments,
                                      const std::optional<RenderPassAttachment>& depthAttachment)
{
    m_passState = PassState{};

    //
    // RenderPassAttachment -> VkRenderingAttachmentInfo
    //
    std::vector<VkRenderingAttachmentInfo> colorAttachmentInfos;
    VkRenderingAttachmentInfo depthAttachmentInfo{};

    std::ranges::transform(colorAttachments, std::back_inserter(colorAttachmentInfos), [this](const auto& colorAttachment) {
        m_passState->renderPassColorAttachments.push_back(colorAttachment);
        return colorAttachment.vkRenderingAttachmentInfo;
    });

    if (depthAttachment)
    {
        m_passState->renderPassDepthAttachment = *depthAttachment;
        depthAttachmentInfo = depthAttachment->vkRenderingAttachmentInfo;
    }

    VkRenderingInfo finalVkRenderingInfo = vkRenderingInfo;
    finalVkRenderingInfo.colorAttachmentCount = (uint32_t)colorAttachmentInfos.size();
    finalVkRenderingInfo.pColorAttachments = colorAttachmentInfos.data();
    if (depthAttachment)
    {
        finalVkRenderingInfo.pDepthAttachment = &depthAttachmentInfo;
    }

    m_vulkanCommandBuffer.CmdBeginRendering(finalVkRenderingInfo);

    // Record usages
    for (const auto& colorAttachment : colorAttachments)
    {
        RecordImageUsage(colorAttachment.gpuImage.imageData.vkImage);
    }

    if (depthAttachment)
    {
        RecordImageUsage(depthAttachment->gpuImage.imageData.vkImage);
    }
}

void CommandBuffer::CmdEndRendering()
{
    m_vulkanCommandBuffer.CmdEndRendering();

    m_passState = std::nullopt;
}

void CommandBuffer::CmdBindPipeline(const VulkanPipeline& vulkanPipeline)
{
    if (!m_passState->BindPipeline(vulkanPipeline)) { return; }

    m_vulkanCommandBuffer.CmdBindPipeline(vulkanPipeline.GetPipelineBindPoint(), vulkanPipeline.GetVkPipeline());

    RecordPipelineUsage(vulkanPipeline.GetVkPipeline());

    for (const auto& vkShaderModule : vulkanPipeline.GetVkShaderModules())
    {
        RecordShaderUsage(vkShaderModule);
    }
}

bool CommandBuffer::CmdBindVertexBuffer(const BufferBinding& bufferBinding)
{
    const auto gpuBuffer = m_pGlobal->pBuffers->GetBuffer(bufferBinding.bufferId, false);
    if (!gpuBuffer)
    {
        m_pGlobal->pLogger->Error("CommandBuffer::CmdBindVertexBuffer: Buffer doesn't exist: {}", bufferBinding.bufferId.id);
        return false;
    }

    const auto vkBufferBinding = VkBufferBinding{
        .gpuBuffer = *gpuBuffer,
        .vkDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .shaderWriteable = false,
        .byteOffset = 0U,
        .byteSize = gpuBuffer->bufferDef.byteSize,
        .dynamicByteOffset = std::nullopt
    };

    if (!m_passState->BindVertexBuffer(vkBufferBinding)) { return true; }

    m_vulkanCommandBuffer.CmdBindVertexBuffers(0, 1, &vkBufferBinding.gpuBuffer.vkBuffer, &vkBufferBinding.byteOffset);

    RecordBufferUsage(vkBufferBinding.gpuBuffer.vkBuffer);

    return true;
}

bool CommandBuffer::CmdBindIndexBuffer(const BufferBinding& bufferBinding, IndexType indexType)
{
    const auto gpuBuffer = m_pGlobal->pBuffers->GetBuffer(bufferBinding.bufferId, false);
    if (!gpuBuffer)
    {
        m_pGlobal->pLogger->Error("CommandBuffer::CmdBindIndexBuffer: Buffer doesn't exist: {}", bufferBinding.bufferId.id);
        return false;
    }

    const auto vkBufferBinding = VkBufferBinding{
        .gpuBuffer = *gpuBuffer,
        .vkDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .shaderWriteable = false,
        .byteOffset = 0U,
        .byteSize = gpuBuffer->bufferDef.byteSize,
        .dynamicByteOffset = std::nullopt
    };

    if (!m_passState->BindIndexBuffer(vkBufferBinding)) { return true; }

    VkIndexType vkIndexType{};
    switch (indexType)
    {
        case IndexType::Uint16: vkIndexType = VK_INDEX_TYPE_UINT16; break;
        case IndexType::Uint32: vkIndexType = VK_INDEX_TYPE_UINT32; break;
    }

    m_vulkanCommandBuffer.CmdBindIndexBuffer(gpuBuffer->vkBuffer, bufferBinding.byteOffset, vkIndexType);

    RecordBufferUsage(gpuBuffer->vkBuffer);

    return true;
}

bool CommandBuffer::CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    m_vulkanCommandBuffer.CmdDrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);

    return true;
}

bool CommandBuffer::CmdDrawIndexedIndirect(VkBuffer vkBuffer, const std::size_t& byteOffset, uint32_t drawCount, uint32_t stride)
{
    m_vulkanCommandBuffer.CmdDrawIndexedIndirect(vkBuffer, byteOffset, drawCount, stride);

    RecordBufferUsage(vkBuffer);

    return true;
}

bool CommandBuffer::CmdDrawIndexedIndirectCount(VkBuffer vkCommandsBuffer,
                                                const std::size_t& commandsByteOffset,
                                                VkBuffer vkCountsBuffer,
                                                const std::size_t& countsByteOffset,
                                                uint32_t maxDrawCount,
                                                uint32_t stride)
{
    m_vulkanCommandBuffer.CmdDrawIndexedIndirectCount(vkCommandsBuffer, commandsByteOffset, vkCountsBuffer, countsByteOffset, maxDrawCount, stride);

    RecordBufferUsage(vkCommandsBuffer);
    RecordBufferUsage(vkCountsBuffer);

    return true;
}

bool CommandBuffer::CmdBindDescriptorSets(const VulkanPipeline& vulkanPipeline,
                                          uint32_t firstSet,
                                          const std::vector<VulkanDescriptorSet>& sets,
                                          const std::vector<uint32_t>& dynamicOffsets)
{
    //
    // Bind the sets
    //
    std::vector<VkDescriptorSet> vkDescriptorSets;

    std::ranges::transform(sets, std::back_inserter(vkDescriptorSets), [](const auto& vulkanDescriptorSet){
       return vulkanDescriptorSet.GetVkDescriptorSet();
    });

    m_vulkanCommandBuffer.CmdBindDescriptorSets(
        vulkanPipeline.GetPipelineBindPoint(),
        vulkanPipeline.GetVkPipelineLayout(),
        firstSet,
        (uint32_t)vkDescriptorSets.size(),
        vkDescriptorSets.data(),
        (uint32_t)dynamicOffsets.size(),
        dynamicOffsets.data()
    );

    //
    // Record this command buffer as using each resource that's bound to the bound sets
    //
    for (const auto& vulkanDescriptorSet : sets)
    {
        RecordDescriptorSetUsage(vulkanDescriptorSet.GetVkDescriptorSet());

        const auto& setBindings = vulkanDescriptorSet.GetSetBindings();

        for (const auto& it : setBindings.bufferBindings)
        {
            RecordBufferUsage(it.second.gpuBuffer.vkBuffer);
        }
        for (const auto& it : setBindings.imageViewBindings)
        {
            RecordImageUsage(it.second.gpuImage.imageData.vkImage);
            RecordImageViewUsage(it.second.gpuImage.imageViewDatas.at(it.second.imageViewIndex).vkImageView);
        }
        for (const auto& it : setBindings.imageViewSamplerBindings)
        {
            for (const auto& binding : it.second.arrayBindings)
            {
                RecordImageUsage(binding.second.gpuImage.imageData.vkImage);
                RecordImageViewUsage(binding.second.gpuImage.imageViewDatas.at(binding.second.imageViewIndex).vkImageView);
            }
        }
    }

    return true;
}

bool CommandBuffer::CmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    m_vulkanCommandBuffer.CmdDispatch(groupCountX, groupCountY, groupCountZ);

    return true;
}

bool CommandBuffer::CmdSetDepthTestEnable(bool enable)
{
    m_vulkanCommandBuffer.CmdSetDepthTestEnable(enable);

    return true;
}

bool CommandBuffer::CmdSetDepthWriteEnable(bool enable)
{
    m_vulkanCommandBuffer.CmdSetDepthWriteEnable(enable);

    return true;
}

bool CommandBuffer::BindBuffer(const std::string& bindPoint, const VkBufferBinding& vkBufferBinding)
{
    m_passState->BindBuffer(bindPoint, vkBufferBinding);

    RecordBufferUsage(vkBufferBinding.gpuBuffer.vkBuffer);

    return true;
}

bool CommandBuffer::BindImageView(const std::string& bindPoint, const VkImageViewBinding& vkImageViewBinding)
{
    m_passState->BindImageView(bindPoint, vkImageViewBinding);

    RecordImageUsage(vkImageViewBinding.gpuImage.imageData.vkImage);
    RecordImageViewUsage(vkImageViewBinding.gpuImage.imageViewDatas.at(vkImageViewBinding.imageViewIndex).vkImageView);

    return true;
}

bool CommandBuffer::BindImageViewSampler(const std::string& bindPoint, uint32_t arrayIndex, const VkImageViewSamplerBinding& vkImageViewSamplerBinding)
{
    m_passState->BindImageViewSampler(bindPoint, arrayIndex, vkImageViewSamplerBinding);

    RecordImageUsage(vkImageViewSamplerBinding.gpuImage.imageData.vkImage);
    RecordImageViewUsage(vkImageViewSamplerBinding.gpuImage.imageViewDatas.at(vkImageViewSamplerBinding.imageViewIndex).vkImageView);
    RecordSamplerUsage(vkImageViewSamplerBinding.vkSampler);

    return true;
}

void CommandBuffer::RecordImageUsage(VkImage vkImage)
{
    if (!m_usedImages.contains(vkImage))
    {
        m_usedImages.insert(vkImage);
        m_pGlobal->pUsages->images.IncrementGPUUsage(vkImage);
    }
}

void CommandBuffer::RecordImageViewUsage(VkImageView vkImageView)
{
    if (!m_usedImageViews.contains(vkImageView))
    {
        m_usedImageViews.insert(vkImageView);
        m_pGlobal->pUsages->imageViews.IncrementGPUUsage(vkImageView);
    }
}

void CommandBuffer::RecordBufferUsage(VkBuffer vkBuffer)
{
    if (!m_usedBuffers.contains(vkBuffer))
    {
        m_usedBuffers.insert(vkBuffer);
        m_pGlobal->pUsages->buffers.IncrementGPUUsage(vkBuffer);
    }
}

void CommandBuffer::RecordPipelineUsage(VkPipeline vkPipeline)
{
    if (!m_usedPipelines.contains(vkPipeline))
    {
        m_usedPipelines.insert(vkPipeline);
        m_pGlobal->pUsages->pipelines.IncrementGPUUsage(vkPipeline);
    }
}

void CommandBuffer::RecordShaderUsage(VkShaderModule vkShaderModule)
{
    if (!m_usedShaders.contains(vkShaderModule))
    {
        m_usedShaders.insert(vkShaderModule);
        m_pGlobal->pUsages->shaders.IncrementGPUUsage(vkShaderModule);
    }
}

void CommandBuffer::RecordDescriptorSetUsage(VkDescriptorSet vkDescriptorSet)
{
    if (!m_usedDescriptorSets.contains(vkDescriptorSet))
    {
        m_usedDescriptorSets.insert(vkDescriptorSet);
        m_pGlobal->pUsages->descriptorSets.IncrementGPUUsage(vkDescriptorSet);
    }
}

void CommandBuffer::RecordSamplerUsage(VkSampler vkSampler)
{
    if (!m_usedSamplers.contains(vkSampler))
    {
        m_usedSamplers.insert(vkSampler);
        m_pGlobal->pUsages->samplers.IncrementGPUUsage(vkSampler);
    }
}

void CommandBuffer::ReleaseTrackedResources()
{
    for (const auto& usedImage : m_usedImages)
    {
        m_pGlobal->pUsages->images.DecrementGPUUsage(usedImage);
    }
    m_usedImages.clear();

    for (const auto& usedImageView : m_usedImageViews)
    {
        m_pGlobal->pUsages->imageViews.DecrementGPUUsage(usedImageView);
    }
    m_usedImageViews.clear();

    for (const auto& usedBuffer : m_usedBuffers)
    {
        m_pGlobal->pUsages->buffers.DecrementGPUUsage(usedBuffer);
    }
    m_usedBuffers.clear();

    for (const auto& usedPipeline : m_usedPipelines)
    {
        m_pGlobal->pUsages->pipelines.DecrementGPUUsage(usedPipeline);
    }
    m_usedPipelines.clear();

    for (const auto& usedShader : m_usedShaders)
    {
        m_pGlobal->pUsages->shaders.DecrementGPUUsage(usedShader);
    }
    m_usedShaders.clear();

    for (const auto& usedDescriptorSet : m_usedDescriptorSets)
    {
        m_pGlobal->pUsages->descriptorSets.DecrementGPUUsage(usedDescriptorSet);
    }
    m_usedDescriptorSets.clear();

    for (const auto& usedSampler : m_usedSamplers)
    {
        m_pGlobal->pUsages->samplers.DecrementGPUUsage(usedSampler);
    }
    m_usedSamplers.clear();
}

}