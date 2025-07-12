/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VkPipelines.h"

#include "../Global.h"
#include "../Usages.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::GPU
{

VkPipelines::VkPipelines(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

VkPipelines::~VkPipelines()
{
    m_pGlobal = nullptr;
}

void VkPipelines::Destroy()
{
    m_pGlobal->pLogger->Info("Pipelines: Destroying");

    std::lock_guard<std::recursive_mutex> lock(m_pipelinesMutex);

    while (!m_pipelines.empty())
    {
        DestroyPipeline(m_pipelines.cbegin()->first, true);
    }
}

std::expected<PipelineId, bool> VkPipelines::CreateGraphicsPipeline(const VkGraphicsPipelineConfig& graphicsPipelineConfig)
{
    std::lock_guard<std::recursive_mutex> lock(m_pipelinesMutex);

    const auto pipelineHash = graphicsPipelineConfig.GetUniqueKey();

    m_pGlobal->pLogger->Info("Pipelines: Creating new graphics pipeline: {}", pipelineHash);

    const auto vulkanPipeline = VulkanPipeline::Create(m_pGlobal, graphicsPipelineConfig);
    if (!vulkanPipeline)
    {
        m_pGlobal->pLogger->Error("Pipelines::CreateGraphicsPipeline: Failed to create new graphics pipeline");
        return std::unexpected(false);
    }

    const auto pipelineId = m_pGlobal->ids.pipelineIds.GetId();

    m_pipelines.insert({pipelineId, *vulkanPipeline});

    return pipelineId;
}

std::expected<PipelineId, bool> VkPipelines::CreateComputePipeline(const VkComputePipelineConfig& computePipelineConfig)
{
    std::lock_guard<std::recursive_mutex> lock(m_pipelinesMutex);

    const auto pipelineHash = computePipelineConfig.GetUniqueKey();

    m_pGlobal->pLogger->Info("Pipelines: Creating new compute pipeline: {}", pipelineHash);

    const auto vulkanPipeline = VulkanPipeline::Create(m_pGlobal, computePipelineConfig);
    if (!vulkanPipeline)
    {
        m_pGlobal->pLogger->Error("Pipelines::CreateComputePipeline: Failed to create new compute pipeline");
        return std::unexpected(false);
    }

    const auto pipelineId = m_pGlobal->ids.pipelineIds.GetId();

    m_pipelines.insert({pipelineId, *vulkanPipeline});

    return pipelineId;
}

std::optional<VulkanPipeline> VkPipelines::GetPipeline(PipelineId pipelineId) const
{
    std::lock_guard<std::recursive_mutex> lock(m_pipelinesMutex);

    const auto it = m_pipelines.find(pipelineId);
    if (it != m_pipelines.cend())
    {
        return it->second;
    }

    return std::nullopt;
}

void VkPipelines::DestroyPipeline(PipelineId pipelineId, bool destroyImmediately)
{
    std::lock_guard<std::recursive_mutex> lock(m_pipelinesMutex);

    const auto it = m_pipelines.find(pipelineId);
    if (it == m_pipelines.cend())
    {
        m_pGlobal->pLogger->Warning("VkPipelines::DestroyPipeline: No such pipeline exists: {}", pipelineId.id);
        return;
    }

    if (destroyImmediately)
    {
        m_pGlobal->pLogger->Debug("VkPipelines::DestroyPipeline: Destroying pipeline: {}", pipelineId.id);

        it->second.Destroy();

        m_pipelines.erase(pipelineId);
        m_pGlobal->ids.pipelineIds.ReturnId(pipelineId);
    }
    else
    {
        m_pipelinesMarkedForDeletion.insert(pipelineId);
        return;
    }
}

void VkPipelines::RunCleanUp()
{
    CleanUp_DeletedPipelines();
}

void VkPipelines::CleanUp_DeletedPipelines()
{
    std::lock_guard<std::recursive_mutex> lock(m_pipelinesMutex);

    std::unordered_set<PipelineId> noLongerMarkedForDeletion;

    for (const auto& pipelineId : m_pipelinesMarkedForDeletion)
    {
        const auto pipeline = m_pipelines.find(pipelineId);
        if (pipeline == m_pipelines.cend())
        {
            m_pGlobal->pLogger->Error("VkPipelines::CleanUp_DeletedPipelines: Pipeline marked for deletion doesn't exist: {}", pipelineId.id);
            noLongerMarkedForDeletion.insert(pipelineId);
            continue;
        }

        // Still in use
        if (m_pGlobal->pUsages->pipelines.GetGPUUsageCount(pipeline->second.GetVkPipeline()) != 0)
        {
            continue;
        }

        DestroyPipeline(pipelineId, true);
        noLongerMarkedForDeletion.insert(pipelineId);
    }

    for (const auto& pipelineId : noLongerMarkedForDeletion)
    {
        m_pipelinesMarkedForDeletion.erase(pipelineId);
    }
}

}
