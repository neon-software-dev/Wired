/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANDESCRIPTORSET_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANDESCRIPTORSET_H

#include "../Common.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>

namespace Wired::GPU
{
    struct Global;

    struct ImageViewSamplerBindings
    {
        // Array index -> binding
        std::unordered_map<uint32_t, VkImageViewSamplerBinding> arrayBindings;
    };

    struct SetBindings
    {
        // Binding index -> binding
        std::unordered_map<uint32_t, VkBufferBinding> bufferBindings;
        std::unordered_map<uint32_t, VkImageViewBinding> imageViewBindings;
        std::unordered_map<uint32_t, ImageViewSamplerBindings> imageViewSamplerBindings;
        // Warning: If these are changed, these places need to be updated:
        // - DescriptorSets::GetHash
        // - DescriptorSets::Reference/DereferenceDescriptorSetUsages
        // - CommandBuffer::CmdBindDescriptorSets
        // - WiredGPUVkImpl::BarrierXSetResourcesYUsage
        // - VulkanDescriptorSet::Write
    };

    class VulkanDescriptorSet
    {
        public:

            struct Hash {
                std::size_t operator()(const VulkanDescriptorSet& o) const {
                    return std::hash<uint64_t>{}((uint64_t)o.m_vkDescriptorSet);
                }
            };

        public:

            VulkanDescriptorSet() = default;
            VulkanDescriptorSet(Global* pGlobal, VkDescriptorSet vkDescriptorSet);
            ~VulkanDescriptorSet();

            [[nodiscard]] bool operator==(const VulkanDescriptorSet& other) const;

            [[nodiscard]] VkDescriptorSet GetVkDescriptorSet() const noexcept { return m_vkDescriptorSet; }
            [[nodiscard]] const SetBindings& GetSetBindings() const noexcept { return m_bindings; }

            void Write(const SetBindings& setBindings);

        private:

            Global* m_pGlobal{nullptr};
            VkDescriptorSet m_vkDescriptorSet{VK_NULL_HANDLE};
            SetBindings m_bindings;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANDESCRIPTORSET_H
