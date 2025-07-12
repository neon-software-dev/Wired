/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PassState.h"

namespace Wired::GPU
{

bool AreSameBufferBinding(const VkBufferBinding& o1, const VkBufferBinding& o2)
{
    if (o1.gpuBuffer.vkBuffer != o2.gpuBuffer.vkBuffer) { return false; }
    if (o1.vkDescriptorType != o2.vkDescriptorType) { return false; }
    if (o1.shaderWriteable != o2.shaderWriteable) { return false; }
    if (o1.byteOffset != o2.byteOffset) { return false; }
    if (o1.byteSize != o2.byteSize) { return false; }
    if (o1.dynamicByteOffset != o2.dynamicByteOffset) { return false; }

    return true;
}

bool AreSameImageViewBinding(const VkImageViewBinding& o1, const VkImageViewBinding& o2)
{
    if (o1.gpuImage.imageData.vkImage != o2.gpuImage.imageData.vkImage) { return false; }
    if (o1.imageViewIndex != o2.imageViewIndex) { return false; }

    return true;
}

bool AreSameImageViewSamplerBinding(const VkImageViewSamplerBinding& o1, const VkImageViewSamplerBinding& o2)
{
    if (o1.gpuImage.imageData.vkImage != o2.gpuImage.imageData.vkImage) { return false; }
    if (o1.imageViewIndex != o2.imageViewIndex) { return false; }
    if (o1.vkSampler != o2.vkSampler) { return false; }

    return true;
}

bool PassState::BindPipeline(const VulkanPipeline& vulkanPipeline)
{
    if (boundPipeline && (boundPipeline->GetVkPipeline() == vulkanPipeline.GetVkPipeline()))
    {
        return false;
    }

    boundPipeline = vulkanPipeline;

    // Mark all sets as invalidated
    InvalidateSet(0);

    // Also reset all set bindings as the new pipeline might have different
    // binding points than a previous pipeline
    setBindings = {};

    // Clear out any bound vertex/index buffer
    boundVertexBuffer = std::nullopt;
    boundIndexBuffer = std::nullopt;

    return true;
}

bool PassState::BindVertexBuffer(const VkBufferBinding& vkBufferBinding)
{
    if (boundVertexBuffer && AreSameBufferBinding(vkBufferBinding, *boundVertexBuffer))
    {
        return false;
    }

    boundVertexBuffer = vkBufferBinding;

    return true;
}

bool PassState::BindIndexBuffer(const VkBufferBinding& vkBufferBinding)
{
    if (boundIndexBuffer && AreSameBufferBinding(vkBufferBinding, *boundIndexBuffer))
    {
        return false;
    }

    boundIndexBuffer = vkBufferBinding;

    return true;
}

void PassState::BindBuffer(const std::string& bindPoint, const VkBufferBinding& bufferBind)
{
    const auto bindingDetails = boundPipeline->GetBindingDetails(bindPoint);
    if (!bindingDetails) { return; }

    const auto bindingIndex = bindingDetails->vkDescriptorSetLayoutBinding.binding;

    // Bail out if we're trying to bind what's already bound
    const auto existingBindIt = setBindings.at(bindingDetails->set).bufferBindings.find(bindingIndex);
    if (existingBindIt != setBindings.at(bindingDetails->set).bufferBindings.cend())
    {
        if (AreSameBufferBinding(existingBindIt->second, bufferBind)) { return; }
    }

    // Mark the data as bound and invalidate the set (and all sets after it)
    setBindings.at(bindingDetails->set).bufferBindings.insert_or_assign(bindingIndex, bufferBind);
    InvalidateSet(bindingDetails->set);
}

void PassState::BindImageView(const std::string& bindPoint, const VkImageViewBinding& imageViewBind)
{
    const auto bindingDetails = boundPipeline->GetBindingDetails(bindPoint);
    if (!bindingDetails) { return; }

    const auto bindingIndex = bindingDetails->vkDescriptorSetLayoutBinding.binding;

    // Bail out if we're trying to bind what's already bound
    const auto existingBindIt = setBindings.at(bindingDetails->set).imageViewBindings.find(bindingIndex);
    if (existingBindIt != setBindings.at(bindingDetails->set).imageViewBindings.cend())
    {
        if (AreSameImageViewBinding(existingBindIt->second, imageViewBind)) { return; }
    }

    // Mark the data as bound and invalidate the set (and all sets after it)
    setBindings.at(bindingDetails->set).imageViewBindings.insert_or_assign(bindingIndex, imageViewBind);
    InvalidateSet(bindingDetails->set);
}

void PassState::BindImageViewSampler(const std::string& bindPoint, uint32_t arrayIndex, const VkImageViewSamplerBinding& imageViewSamplerBind)
{
    const auto bindingDetails = boundPipeline->GetBindingDetails(bindPoint);
    if (!bindingDetails) { return; }

    const auto bindingIndex = bindingDetails->vkDescriptorSetLayoutBinding.binding;

    // Bail out if we're trying to bind what's already bound
    const auto existingBindIt = setBindings.at(bindingDetails->set).imageViewSamplerBindings.find(bindingIndex);
    if (existingBindIt != setBindings.at(bindingDetails->set).imageViewSamplerBindings.cend())
    {
        const auto indexBindIt = existingBindIt->second.arrayBindings.find(arrayIndex);
        if (indexBindIt != existingBindIt->second.arrayBindings.cend())
        {
            if (AreSameImageViewSamplerBinding(indexBindIt->second, imageViewSamplerBind)) { return; }
        }
    }

    // Mark the data as bound and invalidate the set (and all sets after it)
    setBindings.at(bindingDetails->set).imageViewSamplerBindings[bindingIndex].arrayBindings.insert_or_assign(arrayIndex, imageViewSamplerBind);
    InvalidateSet(bindingDetails->set);
}

void PassState::InvalidateSet(uint32_t set)
{
    for (uint32_t x = 0; x < 4; ++x)
    {
        if (set <= x)
        {
            setsNeedingRefresh[x] = true;
        }
    }
}

}
