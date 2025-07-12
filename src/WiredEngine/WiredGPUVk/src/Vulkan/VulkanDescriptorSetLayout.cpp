/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanDescriptorSetLayout.h"
#include "VulkanDebugUtil.h"

#include "../Global.h"

#include <NEON/Common/Log/ILogger.h>

#include <algorithm>

namespace Wired::GPU
{

std::expected<VulkanDescriptorSetLayout, bool> VulkanDescriptorSetLayout::Create(Global* pGlobal,
                                                                                 const std::vector<DescriptorSetLayoutBinding>& bindings,
                                                                                 const std::string& tag)
{
    std::vector<VkDescriptorSetLayoutBinding> vkBindings;
    std::ranges::transform(bindings, std::back_inserter(vkBindings), [](const auto& binding){
        return binding.vkDescriptorSetLayoutBinding;
    });

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = (uint32_t)vkBindings.size();
    layoutInfo.pBindings = vkBindings.data();

    VkDescriptorSetLayout vkDescriptorSetLayout{VK_NULL_HANDLE};
    const auto result = pGlobal->vk.vkCreateDescriptorSetLayout(pGlobal->device.GetVkDevice(), &layoutInfo, nullptr, &vkDescriptorSetLayout);
    if (result != VK_SUCCESS)
    {
        pGlobal->pLogger->Error("VulkanDescriptorSetLayout::Create: Call to vkCreateDescriptorSetLayout() failed, result code: {}", (uint32_t)result);
        return std::unexpected(false);
    }

    SetDebugName(pGlobal->vk, pGlobal->device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)vkDescriptorSetLayout, "DescriptorSetLayout-" + tag);

    return VulkanDescriptorSetLayout(pGlobal, tag, bindings, vkDescriptorSetLayout);
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(Global* pGlobal,
                                                     std::string tag,
                                                     std::vector<DescriptorSetLayoutBinding> bindings,
                                                     VkDescriptorSetLayout vkDescriptorSetLayout)
    : m_pGlobal(pGlobal)
    , m_tag(std::move(tag))
    , m_descriptorSetLayoutBindings(std::move(bindings))
    , m_vkDescriptorSetLayout(vkDescriptorSetLayout)
{

}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
    m_pGlobal = nullptr;
    m_descriptorSetLayoutBindings.clear();
    m_vkDescriptorSetLayout = VK_NULL_HANDLE;
}

void VulkanDescriptorSetLayout::Destroy()
{
    if (m_vkDescriptorSetLayout != VK_NULL_HANDLE)
    {
        RemoveDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)m_vkDescriptorSetLayout);
        m_pGlobal->vk.vkDestroyDescriptorSetLayout(m_pGlobal->device.GetVkDevice(), m_vkDescriptorSetLayout, nullptr);
        m_vkDescriptorSetLayout = VK_NULL_HANDLE;
    }

    m_descriptorSetLayoutBindings.clear();
}

std::vector<VkDescriptorSetLayoutBinding> VulkanDescriptorSetLayout::GetVkDescriptorSetLayoutBindings() const
{
    std::vector<VkDescriptorSetLayoutBinding> vkBindings;
    std::ranges::transform(m_descriptorSetLayoutBindings, std::back_inserter(vkBindings), [](const auto& binding){
        return binding.vkDescriptorSetLayoutBinding;
    });
    return vkBindings;
}

std::optional<DescriptorSetLayoutBinding> VulkanDescriptorSetLayout::GetBindingDetails(const std::string& bindPoint) const
{
    for (const auto& binding : m_descriptorSetLayoutBindings)
    {
        if (binding.bindPoint == bindPoint)
        {
            return binding;
        }
    }

    return std::nullopt;
}

}
