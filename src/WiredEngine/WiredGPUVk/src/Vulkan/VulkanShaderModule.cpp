/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanShaderModule.h"
#include "VulkanDebugUtil.h"

#include "../Global.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::GPU
{

std::expected<VulkanShaderModule, bool> VulkanShaderModule::Create(Global* pGlobal, const ShaderSpec& shaderSpec)
{
    //
    // Use SPIRV-Reflect to parse the shader source and compile details about
    // what inputs, descriptor sets, etc., the shader requires
    //
    SpvReflectShaderModule spvReflectInfo{};
    const auto reflectResult = spvReflectCreateShaderModule(shaderSpec.shaderBinary.size(), shaderSpec.shaderBinary.data(), &spvReflectInfo);
    if (reflectResult != SPV_REFLECT_RESULT_SUCCESS)
    {
        pGlobal->pLogger->Error("VulkanShaderModule::Create: spvReflectCreateShaderModule() call failure, error code: {}", (uint32_t)reflectResult);
        return std::unexpected(false);
    }

    //
    // Create the Vulkan shader module from the shader source
    //
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderSpec.shaderBinary.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderSpec.shaderBinary.data());

    VkShaderModule vkShaderModule{VK_NULL_HANDLE};

    const auto result = pGlobal->vk.vkCreateShaderModule(pGlobal->device.GetVkDevice(), &createInfo, nullptr, &vkShaderModule);
    if (result != VK_SUCCESS)
    {
        pGlobal->pLogger->Error("VulkanShaderModule::Create: vkCreateShaderModule call failure, error code: {}", (uint32_t)result);
        spvReflectDestroyShaderModule(&spvReflectInfo);
        return std::unexpected(false);
    }

    SetDebugName(pGlobal->vk, pGlobal->device, VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)vkShaderModule, std::format("Shader-{}", shaderSpec.shaderName));

    return VulkanShaderModule{pGlobal, shaderSpec, spvReflectInfo, vkShaderModule};
}

VulkanShaderModule::VulkanShaderModule(Global* pGlobal, ShaderSpec shaderSpec, const SpvReflectShaderModule& reflectInfo, VkShaderModule vkShaderModule)
    : m_pGlobal(pGlobal)
    , m_shaderSpec(std::move(shaderSpec))
    , m_spvReflectInfo(reflectInfo)
    , m_vkShaderModule(vkShaderModule)
{

}

VulkanShaderModule::~VulkanShaderModule()
{
    m_pGlobal = nullptr;
    m_shaderSpec = {};
    m_spvReflectInfo = {};
    m_vkShaderModule = VK_NULL_HANDLE;
}

void VulkanShaderModule::Destroy()
{
    if (m_vkShaderModule)
    {
        spvReflectDestroyShaderModule(&m_spvReflectInfo);
        m_spvReflectInfo = {};

        RemoveDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)m_vkShaderModule);
        m_pGlobal->vk.vkDestroyShaderModule(m_pGlobal->device.GetVkDevice(), m_vkShaderModule, nullptr);
        m_vkShaderModule = VK_NULL_HANDLE;
    }
}

}
