/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Shaders.h"

#include "../Global.h"
#include "../Usages.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::GPU
{

Shaders::Shaders(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

Shaders::~Shaders()
{
    m_pGlobal = nullptr;
}

void Shaders::Destroy()
{
    m_pGlobal->pLogger->Info("Shaders: Destroying");

    while (!m_shaders.empty())
    {
        DestroyShader(m_shaders.cbegin()->first, true);
    }

    m_shadersMarkedForDeletion.clear();
}

bool Shaders::CreateShader(const ShaderSpec& shaderSpec)
{
    m_pGlobal->pLogger->Info("Shaders: Creating shader: {}", shaderSpec.shaderName);

    if (shaderSpec.binaryType != ShaderBinaryType::SPIRV)
    {
        m_pGlobal->pLogger->Error("Shaders::CreateGraphicsShader: GPUVk only supports SPIRV shader binaries: {}", shaderSpec.shaderName);
        return false;
    }

    if (GetVulkanShaderModule(shaderSpec.shaderName) != std::nullopt)
    {
        m_pGlobal->pLogger->Error("Shaders::CreateGraphicsShader: Shader module already exists: {}", shaderSpec.shaderName);
        return false;
    }

    const auto vulkanShaderModule = VulkanShaderModule::Create(m_pGlobal, shaderSpec);
    if (!vulkanShaderModule)
    {
        m_pGlobal->pLogger->Error("Shaders::CreateGraphicsShader: Failed to create shader module for: {}", shaderSpec.shaderName);
        return false;
    }

    auto pVulkanShaderModule = std::make_unique<VulkanShaderModule>(*vulkanShaderModule);

    m_shaders.emplace(shaderSpec.shaderName, std::move(pVulkanShaderModule));

    return true;
}

std::optional<VulkanShaderModule*> Shaders::GetVulkanShaderModule(const std::string& shaderName) const
{
    const auto it = m_shaders.find(shaderName);
    if (it == m_shaders.cend())
    {
        return std::nullopt;
    }

    return it->second.get();
}

void Shaders::DestroyShader(const std::string& shaderName, bool destroyImmediately)
{
    const auto pVulkanShaderModule = GetVulkanShaderModule(shaderName);
    if (!pVulkanShaderModule)
    {
        return;
    }

    if (destroyImmediately)
    {
        DestroyShaderObjects(*pVulkanShaderModule);

        m_shaders.erase(shaderName);
    }
    else
    {
        m_shadersMarkedForDeletion.insert(shaderName);
    }
}

void Shaders::DestroyShaderObjects(VulkanShaderModule* pVulkanShaderModule)
{
    m_pGlobal->pLogger->Debug("Shaders: Destroying shader objects: {}", pVulkanShaderModule->GetShaderSpec().shaderName);

    pVulkanShaderModule->Destroy();
}

void Shaders::RunCleanUp()
{
    std::unordered_set<std::string> noLongerMarkedForDeletion;

    for (const auto& shaderName : m_shadersMarkedForDeletion)
    {
        const auto shader = m_shaders.find(shaderName);
        if (shader == m_shaders.cend())
        {
            m_pGlobal->pLogger->Error("Shaders::RunCleanUp: Shader marked for deletion doesn't exist: {}", shaderName);
            noLongerMarkedForDeletion.insert(shaderName);
            continue;
        }

        const bool noUsages = m_pGlobal->pUsages->shaders.GetGPUUsageCount(shader->second->GetVkShaderModule()) == 0;
        const bool noLocks = m_pGlobal->pUsages->shaders.GetLockCount(shader->second->GetVkShaderModule()) == 0;

        if (noUsages && noLocks)
        {
            DestroyShader(shaderName, true);
            noLongerMarkedForDeletion.insert(shaderName);
        }
    }

    for (const auto& shaderName : noLongerMarkedForDeletion)
    {
        m_shadersMarkedForDeletion.erase(shaderName);
    }
}

}
