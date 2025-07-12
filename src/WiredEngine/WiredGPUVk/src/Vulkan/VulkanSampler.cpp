/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanSampler.h"
#include "VulkanDebugUtil.h"

#include "../Global.h"

#include <NEON/Common/Log/ILogger.h>

#include <cassert>

namespace Wired::GPU
{

VkSamplerAddressMode ToVkSamplerAddressMode(SamplerAddressMode samplerAddressMode)
{
    switch (samplerAddressMode)
    {
        case SamplerAddressMode::Clamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case SamplerAddressMode::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case SamplerAddressMode::Mirrored: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    }

    assert(false);
    return {};
}

std::expected<VulkanSampler, bool> VulkanSampler::Create(Global* pGlobal, const SamplerInfo& samplerInfo, const std::string& tag)
{
    VkSamplerCreateInfo vkSamplerCreateInfo{};
    vkSamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    vkSamplerCreateInfo.magFilter = samplerInfo.magFilter == SamplerFilter::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    vkSamplerCreateInfo.minFilter = samplerInfo.minFilter == SamplerFilter::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    vkSamplerCreateInfo.mipmapMode = samplerInfo.mipmapMode == SamplerMipmapMode::Linear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    vkSamplerCreateInfo.addressModeU = ToVkSamplerAddressMode(samplerInfo.addressModeU);
    vkSamplerCreateInfo.addressModeV = ToVkSamplerAddressMode(samplerInfo.addressModeV);
    vkSamplerCreateInfo.addressModeW = ToVkSamplerAddressMode(samplerInfo.addressModeW);
    vkSamplerCreateInfo.compareEnable = VK_FALSE;
    vkSamplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    // Configure anisotropy if requested, and if the device supports it
    if (samplerInfo.anisotropyEnable && pGlobal->physicalDevice.GetPhysicalDeviceFeatures().features.samplerAnisotropy == VK_TRUE)
    {
        const auto anisotropySetting = pGlobal->gpuSettings.samplerAnisotropy;
        const auto maxSamplerAnisotropy = pGlobal->physicalDevice.GetPhysicalDeviceProperties().properties.limits.maxSamplerAnisotropy;

        vkSamplerCreateInfo.anisotropyEnable = VK_TRUE;

        switch(anisotropySetting)
        {
            case SamplerAnisotropy::None:       vkSamplerCreateInfo.maxAnisotropy = 1.0f; break;
            case SamplerAnisotropy::Low:        vkSamplerCreateInfo.maxAnisotropy = std::min(2.0f, maxSamplerAnisotropy); break;
            case SamplerAnisotropy::Maximum:    vkSamplerCreateInfo.maxAnisotropy = maxSamplerAnisotropy; break;
        }
    }
    else
    {
        vkSamplerCreateInfo.anisotropyEnable = VK_FALSE;
        vkSamplerCreateInfo.maxAnisotropy = 0.0f;
    }

    // Apply optional mip configuration
    vkSamplerCreateInfo.mipLodBias = samplerInfo.mipLodBias ? *samplerInfo.mipLodBias : 0.0f;
    vkSamplerCreateInfo.minLod = samplerInfo.minLod ? *samplerInfo.minLod : 0.0f;
    vkSamplerCreateInfo.maxLod = samplerInfo.maxLod ? *samplerInfo.maxLod : VK_LOD_CLAMP_NONE;

    VkSampler vkSampler{VK_NULL_HANDLE};
    const auto result = pGlobal->vk.vkCreateSampler(pGlobal->device.GetVkDevice(), &vkSamplerCreateInfo, nullptr, &vkSampler);
    if (result != VK_SUCCESS)
    {
        pGlobal->pLogger->Error("VulkanSampler::Create: Call to vkCreateSampler() failed, error code: {}", (uint32_t)result);
        return std::unexpected(false);
    }

    SetDebugName(pGlobal->vk, pGlobal->device, VK_OBJECT_TYPE_SAMPLER, (uint64_t)vkSampler, std::format("Sampler-{}", tag));

    return VulkanSampler(pGlobal, vkSampler);
}

VulkanSampler::VulkanSampler(Global* pGlobal, VkSampler vkSampler)
    : m_pGlobal(pGlobal)
    , m_vkSampler(vkSampler)
{

}

VulkanSampler::~VulkanSampler()
{
    m_pGlobal = nullptr;
    m_vkSampler = VK_NULL_HANDLE;
}

void VulkanSampler::Destroy()
{
    if (m_vkSampler != VK_NULL_HANDLE)
    {
        RemoveDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_SAMPLER, (uint64_t)m_vkSampler);
        m_pGlobal->vk.vkDestroySampler(m_pGlobal->device.GetVkDevice(), m_vkSampler, nullptr);
        m_vkSampler = VK_NULL_HANDLE;
    }
}

}
