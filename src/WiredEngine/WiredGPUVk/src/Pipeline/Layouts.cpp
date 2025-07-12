/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Layouts.h"

#include "../Global.h"

#include "../Vulkan/VulkanDebugUtil.h"

#include <NEON/Common/Hash.h>
#include <NEON/Common/Log/ILogger.h>

#include <algorithm>
#include <cassert>

namespace Wired::GPU
{

Layouts::Layouts(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

Layouts::~Layouts()
{
    m_pGlobal = nullptr;
}

void Layouts::Destroy()
{
    m_pGlobal->pLogger->Info("Layouts: Destroying");

    {
        std::lock_guard<std::mutex> lock(m_descriptorSetLayoutsMutex);

        for (auto& it: m_descriptorSetLayouts)
        {
            m_pGlobal->pLogger->Debug("Layouts: Destroying descriptor set layout: {}", it.first);

            it.second.Destroy();
        }
        m_descriptorSetLayouts.clear();
    }

    {
        std::lock_guard<std::mutex> lock(m_pipelineLayoutsMutex);

        for (const auto& it : m_pipelineLayouts)
        {
            m_pGlobal->pLogger->Debug("Layouts: Destroying pipeline layout: {}", it.first);

            RemoveDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)it.second);
            m_pGlobal->vk.vkDestroyPipelineLayout(m_pGlobal->device.GetVkDevice(), it.second, nullptr);
        }
        m_pipelineLayouts.clear();
    }
}

std::expected<VulkanDescriptorSetLayout, bool> Layouts::GetOrCreateDescriptorSetLayout(const std::vector<DescriptorSetLayoutBinding>& bindings,
                                                                                       const std::string& tag)
{
    std::vector<std::size_t> bindingHashes;

    for (const auto& binding : bindings)
    {
        bindingHashes.push_back(NCommon::Hash(
            binding.set,
            binding.bindPoint,
            binding.vkDescriptorSetLayoutBinding.binding,
            binding.vkDescriptorSetLayoutBinding.descriptorType,
            binding.vkDescriptorSetLayoutBinding.descriptorCount,
            binding.vkDescriptorSetLayoutBinding.stageFlags,
            binding.vkDescriptorSetLayoutBinding.pImmutableSamplers
        ));
    }

    std::size_t hash = 0;
    for (const auto& bindingHash : bindingHashes)
    {
        NCommon::HashCombine(hash, bindingHash);
    }

    std::lock_guard<std::mutex> lock(m_descriptorSetLayoutsMutex);

    const auto it = m_descriptorSetLayouts.find(hash);
    if (it != m_descriptorSetLayouts.cend())
    {
        return it->second;
    }

    const auto vulkanDescriptorSetLayout = VulkanDescriptorSetLayout::Create(m_pGlobal, bindings, tag);
    if (!vulkanDescriptorSetLayout)
    {
        m_pGlobal->pLogger->Error("Layouts::GetOrCreateDescriptorSetLayout: Failed to create new vulkan descriptor set layout");
        return std::unexpected(false);
    }

    m_descriptorSetLayouts.insert({hash, *vulkanDescriptorSetLayout});

    return *vulkanDescriptorSetLayout;
}

std::expected<VkPipelineLayout, bool> Layouts::GetOrCreatePipelineLayout(const std::array<VkDescriptorSetLayout, 4>& descriptorSetLayouts,
                                                                         const std::vector<VkPushConstantRange>& pushConstantRanges,
                                                                         const std::string& tag)
{
    std::size_t hash = 0;
    for (const auto& descriptorSetLayout : descriptorSetLayouts)
    {
        NCommon::HashCombine(hash, descriptorSetLayout);
    }

    for (const auto& pushConstantRange : pushConstantRanges)
    {
        if ((pushConstantRange.offset) % 4 != 0)
        {
            m_pGlobal->pLogger->Error("Layouts::GetOrCreatePipelineLayout: Push constant offset must be a multiple of 4: {}", pushConstantRange.offset);
            return std::unexpected(false);
        }
        if ((pushConstantRange.size) % 4 != 0)
        {
            m_pGlobal->pLogger->Error("Layouts::GetOrCreatePipelineLayout: Push constant size must be a multiple of 4: {}", pushConstantRange.size);
            return std::unexpected(false);
        }

        NCommon::HashCombine(hash, NCommon::Hash(hash, pushConstantRange.stageFlags, pushConstantRange.offset, pushConstantRange.stageFlags));
    }

    std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts;
    std::ranges::transform(descriptorSetLayouts, std::back_inserter(vkDescriptorSetLayouts), std::identity{});

    std::lock_guard<std::mutex> lock(m_pipelineLayoutsMutex);

    const auto it = m_pipelineLayouts.find(hash);
    if (it != m_pipelineLayouts.cend())
    {
        return it->second;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = (uint32_t)pushConstantRanges.size();
    pipelineLayoutInfo.pPushConstantRanges = !pushConstantRanges.empty() ? pushConstantRanges.data() : nullptr;
    pipelineLayoutInfo.setLayoutCount = (uint32_t)vkDescriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts = vkDescriptorSetLayouts.data();

    VkPipelineLayout vkPipelineLayout{VK_NULL_HANDLE};
    auto result = m_pGlobal->vk.vkCreatePipelineLayout(m_pGlobal->device.GetVkDevice(), &pipelineLayoutInfo, nullptr, &vkPipelineLayout);
    if (result != VK_SUCCESS)
    {
        m_pGlobal->pLogger->Error("Layouts::GetOrCreatePipelineLayout: Call to vkCreatePipelineLayout() failed, result code: {}", (uint32_t)result);
        return std::unexpected(false);
    }

    SetDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)vkPipelineLayout, std::format("PipelineLayout-{}", tag));

    m_pipelineLayouts.insert({hash, vkPipelineLayout});

    return vkPipelineLayout;
}

}
