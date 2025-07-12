/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANCOMMANDBUFFER_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANCOMMANDBUFFER_H

#include "../Util/SyncPrimitives.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <string>

namespace Wired::GPU
{
    struct Global;

    enum class CommandBufferType
    {
        Primary,
        Secondary
    };

    class VulkanCommandBuffer
    {
        public:

            struct Hash {
                std::size_t operator()(const VulkanCommandBuffer& o) const {
                    return std::hash<uint64_t>{}((uint64_t)o.m_vkCommandBuffer);
                }
            };

        public:

            VulkanCommandBuffer() = default;
            VulkanCommandBuffer(Global* pGlobal, CommandBufferType commandBufferType, VkCommandBuffer vkCommandBuffer, std::string tag);
            ~VulkanCommandBuffer();

            [[nodiscard]] bool operator==(const VulkanCommandBuffer& other) const;

            [[nodiscard]] CommandBufferType GetCommandBufferType() const noexcept { return m_commandBufferType; }
            [[nodiscard]] VkCommandBuffer GetVkCommandBuffer() const noexcept { return m_vkCommandBuffer; }
            [[nodiscard]] bool IsValid() const noexcept { return m_vkCommandBuffer != VK_NULL_HANDLE; }

            void Begin(const VkCommandBufferUsageFlagBits& flags) const;
            void End() const;
            void CmdPipelineBarrier2(const Barrier& barrier) const;
            void CmdClearColorImage(VkImage vkImage, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges) const;
            void CmdBlitImage(VkImage vkSourceImage, VkImageLayout vkSourceImageLayout, VkImage vkDestImage, VkImageLayout vkDestImageLayout, VkImageBlit vkImageBlit, VkFilter vkFilter) const;
            void CmdExecuteCommands(const std::vector<VulkanCommandBuffer>& commands) const;
            void CmdCopyBuffer2(const VkCopyBufferInfo2* pCopyBufferInfo) const;
            void CmdCopyBufferToImage2(const VkCopyBufferToImageInfo2* pCopyBufferToImageInfo) const;
            void CmdBeginRendering(const VkRenderingInfo& vkRenderingInfo) const;
            void CmdEndRendering();
            void CmdBindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) const;
            void CmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets) const;
            void CmdBindIndexBuffer(VkBuffer vkBuffer, const std::size_t& byteOffset, VkIndexType vkIndexType) const;
            void CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const;
            void CmdDrawIndexedIndirect(VkBuffer vkBuffer, const std::size_t& byteOffset, uint32_t drawCount, uint32_t stride) const;
            void CmdDrawIndexedIndirectCount(VkBuffer vkCommandsBuffer, const std::size_t& commandsByteOffset, VkBuffer vkCountsBuffer, const std::size_t& countsByteOffset, uint32_t maxDrawCount, uint32_t stride) const;
            void CmdBindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets) const;
            void CmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
            void CmdSetDepthTestEnable(bool enable);
            void CmdSetDepthWriteEnable(bool enable);

        private:

            Global* m_pGlobal{nullptr};
            CommandBufferType m_commandBufferType{};
            VkCommandBuffer m_vkCommandBuffer{VK_NULL_HANDLE};
            std::string m_tag;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANCOMMANDBUFFER_H
