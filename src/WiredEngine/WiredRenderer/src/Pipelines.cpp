/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Pipelines.h"
#include "Global.h"

#include "Wired/GPU/WiredGPU.h"

#include <NEON/Common/Log/ILogger.h>

#include <NEON/Common/Hash.h>

namespace Wired::Render
{

Pipelines::Pipelines(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

Pipelines::~Pipelines()
{
    m_pGlobal = nullptr;
}

void Pipelines::ShutDown()
{
    m_pGlobal->pLogger->Info("Pipelines: Shutting down");

    for (const auto& it : m_pipelines)
    {
        m_pGlobal->pGPU->DestroyPipeline(it.second);
    }
    m_pipelines.clear();
}

std::expected<GPU::PipelineId, bool> Pipelines::GetOrCreatePipeline(const GPU::GraphicsPipelineParams& graphicsPipelineParams)
{
    const auto paramsHash = graphicsPipelineParams.GetHash();

    const auto it = m_pipelines.find(paramsHash);
    if (it != m_pipelines.cend())
    {
        return it->second;
    }

    const auto pipelineId = m_pGlobal->pGPU->CreateGraphicsPipeline(graphicsPipelineParams);
    if (!pipelineId)
    {
        m_pGlobal->pLogger->Error("Pipelines::GetOrCreatePipeline: Failed to create graphics pipeline");
        return std::unexpected(false);
    }

    m_pipelines.insert({paramsHash, *pipelineId});

    return pipelineId;
}

std::expected<GPU::PipelineId, bool> Pipelines::GetOrCreatePipeline(const GPU::ComputePipelineParams& computePipelineParams)
{
    const auto paramsHash = computePipelineParams.GetHash();

    const auto it = m_pipelines.find(paramsHash);
    if (it != m_pipelines.cend())
    {
        return it->second;
    }

    const auto pipelineId = m_pGlobal->pGPU->CreateComputePipeline(computePipelineParams);
    if (!pipelineId)
    {
        m_pGlobal->pLogger->Error("Pipelines::GetOrCreatePipeline: Failed to create compute pipeline");
        return std::unexpected(false);
    }

    m_pipelines.insert({paramsHash, *pipelineId});

    return pipelineId;
}

std::string Pipelines::GetShaderNameFromBaseName(const std::string& shaderBaseName) const
{
    std::string extension;
    switch (m_pGlobal->shaderBinaryType)
    {
        case GPU::ShaderBinaryType::SPIRV: { extension = "spv"; }
    }

    return std::format("{}.{}", shaderBaseName, extension);
}

}
