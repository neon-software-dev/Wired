/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_RENDERPASSSATE_H
#define WIREDENGINE_WIREDGPUVK_SRC_RENDERPASSSATE_H

#include "Common.h"

#include "Util/RenderPassAttachment.h"

#include "Vulkan/VulkanPipeline.h"
#include "Vulkan/VulkanDescriptorSet.h"

#include <Wired/GPU/GPUCommon.h>

#include <vulkan/vulkan.h>

#include <vector>
#include <optional>
#include <unordered_map>
#include <string>
#include <array>

namespace Wired::GPU
{
    struct PassState
    {
        [[nodiscard]] bool BindPipeline(const VulkanPipeline& vulkanPipeline);
        [[nodiscard]] bool BindVertexBuffer(const VkBufferBinding& vkBufferBinding);
        [[nodiscard]] bool BindIndexBuffer(const VkBufferBinding& vkBufferBinding);

        void BindBuffer(const std::string& bindPoint, const VkBufferBinding& bufferBind);
        void BindImageView(const std::string& bindPoint, const VkImageViewBinding& imageViewBind);
        void BindImageViewSampler(const std::string& bindPoint, uint32_t arrayIndex, const VkImageViewSamplerBinding& imageViewSamplerBind);

        //
        // Attachments being rendered into (render pass)
        //
        std::vector<RenderPassAttachment> renderPassColorAttachments;
        std::optional<RenderPassAttachment> renderPassDepthAttachment;

        //
        // Bind State
        //
        std::optional<VulkanPipeline> boundPipeline;
        std::optional<VkBufferBinding> boundVertexBuffer;
        std::optional<VkBufferBinding> boundIndexBuffer;

        std::array<bool, 4> setsNeedingRefresh{true};
        std::array<SetBindings, 4> setBindings{};

        private:

            void InvalidateSet(uint32_t set);
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_RENDERPASSSATE_H
