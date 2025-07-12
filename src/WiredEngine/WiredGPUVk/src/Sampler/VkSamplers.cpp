/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VkSamplers.h"

#include "../Global.h"
#include "../Usages.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::GPU
{

VkSamplers::VkSamplers(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

VkSamplers::~VkSamplers()
{
    m_pGlobal = nullptr;
}

void VkSamplers::Destroy()
{
    m_pGlobal->pLogger->Info("Samplers: Destroying");

    while (!m_samplers.empty())
    {
        DestroySampler(m_samplers.cbegin()->first, true);
    }
}

std::expected<SamplerId, bool> VkSamplers::CreateSampler(const SamplerInfo& samplerInfo, const std::string& tag)
{
    const auto vulkanSampler = VulkanSampler::Create(m_pGlobal, samplerInfo, tag);
    if (!vulkanSampler)
    {
        m_pGlobal->pLogger->Error("Samplers::CreateSampler: Failed to create sampler: {}", tag);
        return std::unexpected(false);
    }

    const auto samplerId = m_pGlobal->ids.samplerIds.GetId();

    m_samplers.insert({samplerId, *vulkanSampler});

    return samplerId;
}

void VkSamplers::DestroySampler(SamplerId samplerId, bool destroyImmediately)
{
    const auto it = m_samplers.find(samplerId);
    if (it == m_samplers.cend())
    {
        return;
    }

    if (destroyImmediately)
    {
        m_pGlobal->pLogger->Debug("Samplers: Destroying sampler: {}", samplerId.id);

        it->second.Destroy();

        m_samplers.erase(samplerId);
        m_pGlobal->ids.samplerIds.ReturnId(samplerId);
    }
    else
    {
        m_samplersMarkedForDeletion.insert(samplerId);
    }
}

void VkSamplers::RunCleanUp()
{
    CleanUp_DeletedSamplers();
}

void VkSamplers::CleanUp_DeletedSamplers()
{
    std::unordered_set<SamplerId> noLongerMarkedForDeletion;

    for (const auto& samplerId : m_samplersMarkedForDeletion)
    {
        const auto sampler = m_samplers.find(samplerId);
        if (sampler == m_samplers.cend())
        {
            m_pGlobal->pLogger->Error("Samplers::RunCleanUp: Sampler marked for deletion doesn't exist: {}", samplerId.id);
            noLongerMarkedForDeletion.insert(samplerId);
            continue;
        }

        if (m_pGlobal->pUsages->samplers.GetGPUUsageCount(sampler->second.GetVkSampler()) != 0)
        {
            continue;
        }

        DestroySampler(samplerId, true);
        noLongerMarkedForDeletion.insert(samplerId);
    }

    for (const auto& samplerId : noLongerMarkedForDeletion)
    {
        m_samplers.erase(samplerId);
    }
}

std::optional<VulkanSampler> VkSamplers::GetSampler(SamplerId samplerId)
{
    const auto it = m_samplers.find(samplerId);
    if (it == m_samplers.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

}
