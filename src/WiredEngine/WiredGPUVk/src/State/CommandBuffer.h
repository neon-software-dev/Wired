/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_STATE_COMMANDBUFFER_H
#define WIREDENGINE_WIREDGPUVK_SRC_STATE_COMMANDBUFFER_H

#include "../PassState.h"

#include "../Image/GPUImage.h"
#include "../Buffer/GPUBuffer.h"

#include "../Vulkan/VulkanCommandPool.h"
#include "../Vulkan/VulkanPipeline.h"

#include "../Util/RenderPassAttachment.h"

#include <Wired/GPU/GPUId.h>

#include <vulkan/vulkan.h>

#include <expected>
#include <vector>
#include <optional>
#include <cassert>
#include <unordered_set>

namespace Wired::GPU
{
    struct Global;

    enum class CommandBufferState
    {
        Default,
        CopyPass,
        RenderPass,
        ComputePass
    };

    class CommandBuffer
    {
        public:

            [[nodiscard]] static std::expected<CommandBuffer, bool> Create(Global* pGlobal,
                                                                           VulkanCommandPool* pVulkanCommandPool,
                                                                           CommandBufferType type,
                                                                           const std::string& tag);

        public:

            CommandBuffer(Global* pGlobal,
                          std::string tag,
                          CommandBufferType type,
                          CommandBufferId commandBufferId,
                          VulkanCommandPool* pVulkanCommandPool,
                          const VulkanCommandBuffer& vulkanCommandBuffer,
                          VkFence vkFence);

            ~CommandBuffer();

            void Destroy();

            [[nodiscard]] std::string GetTag() const noexcept { return m_tag; }
            [[nodiscard]] CommandBufferType GetType() const noexcept { return m_type; }
            [[nodiscard]] CommandBufferId GetId() const noexcept { return m_id; }
            [[nodiscard]] VulkanCommandBuffer& GetVulkanCommandBuffer() { return m_vulkanCommandBuffer; }

            // Specific to primary command buffers
            [[nodiscard]] VkFence GetVkFence() const noexcept { assert(m_type == CommandBufferType::Primary); return m_vkFence; }
            void ConfigureForPresentation(SemaphoreOp waitOn, SemaphoreOp signalOn);
            [[nodiscard]] bool IsConfiguredForPresentation() const noexcept { assert(m_type == CommandBufferType::Primary); return m_configuredForPresent; }
            [[nodiscard]] std::vector<SemaphoreOp> GetSignalSemaphores() const noexcept { assert(m_type == CommandBufferType::Primary); return m_signalSemaphores; }
            [[nodiscard]] std::vector<SemaphoreOp> GetWaitSemaphores() const noexcept { assert(m_type == CommandBufferType::Primary);  return m_waitSemaphores; }
            [[nodiscard]] std::unordered_set<CommandBufferId> GetSecondaryCommandBufferIds() const noexcept { return m_secondaryCommandBuffers; }

            // Specific to render pass state
            [[nodiscard]] std::optional<PassState>& GetPassState() noexcept { return m_passState; }

            // Commands
            [[nodiscard]] bool IsInAnyPass();
            [[nodiscard]] bool BeginCopyPass();
            [[nodiscard]] bool EndCopyPass();
            [[nodiscard]] bool IsInCopyPass();
            [[nodiscard]] bool BeginRenderPass();
            [[nodiscard]] bool EndRenderPass();
            [[nodiscard]] bool IsInRenderPass();
            [[nodiscard]] bool BeginComputePass();
            [[nodiscard]] bool EndComputePass();
            [[nodiscard]] bool IsInComputePass();

            void CmdImagePipelineBarrier(const GPUImage& gpuImage, VkImageSubresourceRange vkImageSubresourceRange, ImageUsageMode sourceUsageMode, ImageUsageMode destUsageMode);
            void CmdBufferPipelineBarrier(const GPUBuffer& gpuBuffer, const std::size_t& byteOffset, const std::size_t& byteSize, BufferUsageMode sourceUsageMode, BufferUsageMode destUsageMode);
            void CmdClearColorImage(const GPUImage& loadedImage, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
            void CmdBlitImage(const GPUImage& sourceImage, VkImageLayout sourceImageLayout, const GPUImage& destImage, VkImageLayout destImageLayout, VkImageBlit vkImageBlit, VkFilter vkFilter);
            void CmdExecuteCommands(const std::vector<CommandBuffer*>& secondaryCommandBuffers);
            void CmdCopyBuffer2(const VkCopyBufferInfo2* pCopyBufferInfo);
            void CmdCopyBufferToImage2(const VkCopyBufferToImageInfo2* pCopyBufferToImageInfo);
            void CmdBeginRendering(const VkRenderingInfo& vkRenderingInfo, const std::vector<RenderPassAttachment>& colorAttachments, const std::optional<RenderPassAttachment>& depthAttachment);
            void CmdEndRendering();
            void CmdBindPipeline(const VulkanPipeline& vulkanPipeline);
            bool CmdBindVertexBuffer(const BufferBinding& bufferBinding);
            bool CmdBindIndexBuffer(const BufferBinding& bufferBinding, IndexType indexType);
            bool CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
            bool CmdDrawIndexedIndirect(VkBuffer vkBuffer, const std::size_t& byteOffset, uint32_t drawCount, uint32_t stride);
            bool CmdDrawIndexedIndirectCount(VkBuffer vkCommandsBuffer, const std::size_t& commandsByteOffset, VkBuffer vkCountsBuffer, const std::size_t& countsByteOffset, uint32_t maxDrawCount, uint32_t stride);
            bool CmdBindDescriptorSets(const VulkanPipeline& vulkanPipeline, uint32_t firstSet, const std::vector<VulkanDescriptorSet>& sets, const std::vector<uint32_t>& dynamicOffsets);
            bool CmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
            bool CmdSetDepthTestEnable(bool enable);
            bool CmdSetDepthWriteEnable(bool enable);

            bool BindBuffer(const std::string& bindPoint, const VkBufferBinding& vkBufferBinding);
            bool BindImageView(const std::string& bindPoint, const VkImageViewBinding& vkImageViewBinding);
            bool BindImageViewSampler(const std::string& bindPoint, uint32_t arrayIndex, const VkImageViewSamplerBinding& vkImageViewSamplerBinding);

            // Resource tracking
            void ReleaseTrackedResources();

        private:

            void RecordImageUsage(VkImage vkImage);
            void RecordImageViewUsage(VkImageView vkImageView);
            void RecordBufferUsage(VkBuffer vkBuffer);
            void RecordPipelineUsage(VkPipeline vkPipeline);
            void RecordShaderUsage(VkShaderModule vkShaderModule);
            void RecordDescriptorSetUsage(VkDescriptorSet vkDescriptorSet);
            void RecordSamplerUsage(VkSampler vkSampler);

        private:

            Global* m_pGlobal;
            std::string m_tag;
            CommandBufferType m_type;
            CommandBufferId m_id;
            VulkanCommandPool* m_pVulkanCommandPool;
            VulkanCommandBuffer m_vulkanCommandBuffer;

            // Specific to primary command buffers
            VkFence m_vkFence;
            std::vector<SemaphoreOp> m_signalSemaphores;
            std::vector<SemaphoreOp> m_waitSemaphores;
            bool m_configuredForPresent{false};
            std::unordered_set<CommandBufferId> m_secondaryCommandBuffers;

            std::unordered_set<VkImage> m_usedImages;
            std::unordered_set<VkImageView> m_usedImageViews;
            std::unordered_set<VkBuffer> m_usedBuffers;
            std::unordered_set<VkPipeline> m_usedPipelines;
            std::unordered_set<VkShaderModule> m_usedShaders;
            std::unordered_set<VkDescriptorSet> m_usedDescriptorSets;
            std::unordered_set<VkSampler> m_usedSamplers;

            CommandBufferState m_state{CommandBufferState::Default};

            std::optional<PassState> m_passState;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_STATE_COMMANDBUFFER_H
