/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanDescriptorSet.h"

#include "../Global.h"
#include "../Usages.h"

namespace Wired::GPU
{

VulkanDescriptorSet::VulkanDescriptorSet(Global* pGlobal, VkDescriptorSet vkDescriptorSet)
    : m_pGlobal(pGlobal)
    , m_vkDescriptorSet(vkDescriptorSet)
{

}

VulkanDescriptorSet::~VulkanDescriptorSet()
{
    m_pGlobal = nullptr;
    m_vkDescriptorSet = VK_NULL_HANDLE;
    m_bindings = {};
}

bool VulkanDescriptorSet::operator==(const VulkanDescriptorSet& other) const
{
    return m_vkDescriptorSet == other.m_vkDescriptorSet;
}

void VulkanDescriptorSet::Write(const SetBindings& setBindings)
{
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;

    //
    // Create Info objects for each buffer/image to be bound
    //
    for (const auto& it : setBindings.bufferBindings)
    {
        const auto& vkBufferBinding = it.second;

        bufferInfos.push_back(VkDescriptorBufferInfo{
            .buffer = vkBufferBinding.gpuBuffer.vkBuffer,
            .offset = vkBufferBinding.byteOffset,
            .range = vkBufferBinding.byteSize == 0 ? VK_WHOLE_SIZE : vkBufferBinding.byteSize
        });
    }

    for (const auto& it : setBindings.imageViewBindings)
    {
        const auto& vkImageViewBinding = it.second;

        imageInfos.push_back(VkDescriptorImageInfo{
            .sampler = VK_NULL_HANDLE,
            .imageView = vkImageViewBinding.gpuImage.imageViewDatas.at(vkImageViewBinding.imageViewIndex).vkImageView,
            .imageLayout = vkImageViewBinding.shaderWriteable ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
        });
    }

    for (const auto& it : setBindings.imageViewSamplerBindings)
    {
        const auto& vkImageViewSamplerBindings = it.second;

        for (const auto& bindingIt : vkImageViewSamplerBindings.arrayBindings)
        {
            imageInfos.push_back(VkDescriptorImageInfo{
                .sampler = bindingIt.second.vkSampler,
                .imageView = bindingIt.second.gpuImage.imageViewDatas.at(bindingIt.second.imageViewIndex).vkImageView,
                .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
            });
        }
    }

    //
    // Generate writes for each binding to be updated
    //
    std::vector<VkWriteDescriptorSet> vkWrites;
    std::size_t bufferInfoIndex = 0;
    std::size_t imageInfoIndex = 0;

    for (const auto& it : setBindings.bufferBindings)
    {
        const auto& bindingIndex = it.first;

        VkWriteDescriptorSet vkDescriptorWrite{};
        vkDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vkDescriptorWrite.dstSet = m_vkDescriptorSet;
        vkDescriptorWrite.dstBinding = bindingIndex;
        vkDescriptorWrite.dstArrayElement = 0;
        vkDescriptorWrite.descriptorType = it.second.vkDescriptorType;
        vkDescriptorWrite.descriptorCount = 1;
        vkDescriptorWrite.pBufferInfo = &bufferInfos.at(bufferInfoIndex);
        vkDescriptorWrite.pImageInfo = nullptr;
        vkDescriptorWrite.pTexelBufferView = nullptr;

        vkWrites.push_back(vkDescriptorWrite);
        m_bindings.bufferBindings.insert_or_assign(it.first, it.second);

        bufferInfoIndex += vkDescriptorWrite.descriptorCount;
    }

    for (const auto& it : setBindings.imageViewBindings)
    {
        const auto& bindingIndex = it.first;

        VkWriteDescriptorSet vkDescriptorWrite{};
        vkDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vkDescriptorWrite.dstSet = m_vkDescriptorSet;
        vkDescriptorWrite.dstBinding = bindingIndex;
        vkDescriptorWrite.dstArrayElement = 0;
        vkDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        vkDescriptorWrite.descriptorCount = 1;
        vkDescriptorWrite.pBufferInfo = nullptr;
        vkDescriptorWrite.pImageInfo = &imageInfos.at(imageInfoIndex);
        vkDescriptorWrite.pTexelBufferView = nullptr; // Optional

        vkWrites.push_back(vkDescriptorWrite);
        m_bindings.imageViewBindings.insert_or_assign(it.first, it.second);

        imageInfoIndex += vkDescriptorWrite.descriptorCount;
    }

    for (const auto& it : setBindings.imageViewSamplerBindings)
    {
        const auto& bindingIndex = it.first;
        const auto& vkImageViewSamplerBindings = it.second;

        if (!m_bindings.imageViewSamplerBindings.contains(bindingIndex))
        {
            m_bindings.imageViewSamplerBindings[bindingIndex] = ImageViewSamplerBindings{};
        }
        auto& boundImageViewSamplerBindings = m_bindings.imageViewSamplerBindings.at(bindingIndex);

        for (const auto& arrayBindingIt : vkImageViewSamplerBindings.arrayBindings)
        {
            const auto& arrayIndex = arrayBindingIt.first;
            const auto& binding = arrayBindingIt.second;

            VkWriteDescriptorSet vkDescriptorWrite{};
            vkDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            vkDescriptorWrite.dstSet = m_vkDescriptorSet;
            vkDescriptorWrite.dstBinding = bindingIndex;
            vkDescriptorWrite.dstArrayElement = arrayIndex;
            vkDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            vkDescriptorWrite.descriptorCount = 1;
            vkDescriptorWrite.pBufferInfo = nullptr;
            vkDescriptorWrite.pImageInfo = &imageInfos.at(imageInfoIndex);
            vkDescriptorWrite.pTexelBufferView = nullptr; // Optional

            vkWrites.push_back(vkDescriptorWrite);

            imageInfoIndex += vkDescriptorWrite.descriptorCount;

            boundImageViewSamplerBindings.arrayBindings.insert_or_assign(arrayIndex, binding);
        }
    }

    //
    // Update the descriptor set
    //
    m_pGlobal->vk.vkUpdateDescriptorSets(m_pGlobal->device.GetVkDevice(), (uint32_t)vkWrites.size(), vkWrites.data(), 0, nullptr);
}

}
