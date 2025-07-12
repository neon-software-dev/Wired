/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Timestamps.h"
#include "Global.h"

#include "State/CommandBuffer.h"

namespace Wired::GPU
{

bool Timestamps::QueueFamilySupportsTimestampQueries(Global* pGlobal, uint32_t queueFamilyIndex)
{
    return VulkanQueryPool::QueueFamilySupportsTimestampQueries(pGlobal, queueFamilyIndex);
}

std::expected<std::unique_ptr<Timestamps>, bool> Timestamps::Create(Global* pGlobal, const std::string& tag)
{
    const auto queryPool = VulkanQueryPool::Create(pGlobal, pGlobal->gpuSettings.numTimestamps, tag);
    if (!queryPool)
    {
        return std::unexpected(false);
    }

    return std::make_unique<Timestamps>(pGlobal, *queryPool);
}

Timestamps::Timestamps(Global* pGlobal, const VulkanQueryPool& queryPool)
    : m_pGlobal(pGlobal)
    , m_queryPool(queryPool)
{
    m_timestampPeriod = m_pGlobal->physicalDevice.GetPhysicalDeviceProperties().properties.limits.timestampPeriod;

    m_timestampRawData.resize(m_queryPool.GetNumTimestamps(), 0);
}

void Timestamps::Destroy()
{
    m_queryPool.Destroy();
    m_initialResetDone = false;
    m_freeIndex = 0;
    m_timestampToIndex.clear();
    m_timestampRawData = {};
}

void Timestamps::SyncDownTimestamps()
{
    QueryWrittenTimestamps();
}

void Timestamps::ResetForRecording(CommandBuffer* pCommandBuffer)
{
    ResetQueryPool(pCommandBuffer);

    m_timestampRawData = {};
    m_timestampRawData.resize(m_queryPool.GetNumTimestamps(), 0);

    m_freeIndex = 0;
    m_timestampToIndex.clear();
}

void Timestamps::QueryWrittenTimestamps()
{
    if (!m_initialResetDone) { return; }

    m_pGlobal->vk.vkGetQueryPoolResults(
        m_pGlobal->device.GetVkDevice(),
        m_queryPool.GetVkQueryPool(),
        0, // firstQuery
        m_freeIndex, // queryCount
        sizeof(uint64_t), // dataSize
        m_timestampRawData.data(), // pData
        sizeof(uint64_t), // stride
        // Don't need to VK_QUERY_RESULT_WAIT_BIT because at the moment this func is called after cpu<=>gpu fence sync
        VK_QUERY_RESULT_64_BIT// | VK_QUERY_RESULT_WAIT_BIT
    );
}

void Timestamps::ResetQueryPool(CommandBuffer* pCommandBuffer)
{
    m_pGlobal->vk.vkCmdResetQueryPool(
        pCommandBuffer->GetVulkanCommandBuffer().GetVkCommandBuffer(),
        m_queryPool.GetVkQueryPool(),
        0,
        m_queryPool.GetNumTimestamps()
    );

    m_initialResetDone = true;
}

void Timestamps::WriteTimestampStart(CommandBuffer* pCommandBuffer, const std::string& name, uint32_t timestampSpan)
{
    // The * 2s in here are to reserve indices for both the timestamp starts and finishes

    if (m_freeIndex + (timestampSpan * 2) > m_queryPool.GetNumTimestamps())
    {
        m_pGlobal->pLogger->Error(
            "Timestamps::WriteTimestampStart: Ran out of timestamps, unable to record timestamp: {}", name);
        return;
    }

    m_pGlobal->vk.vkCmdWriteTimestamp2(
        pCommandBuffer->GetVulkanCommandBuffer().GetVkCommandBuffer(),
        VK_PIPELINE_STAGE_2_NONE,
        m_queryPool.GetVkQueryPool(),
        m_freeIndex
    );

    m_timestampToIndex.insert_or_assign(name, TimestampTracking{.index = m_freeIndex, .span = timestampSpan});
    m_freeIndex += (timestampSpan * 2);
}

void Timestamps::WriteTimestampFinish(CommandBuffer* pCommandBuffer, const std::string& name)
{
    const auto it = m_timestampToIndex.find(name);
    if (it == m_timestampToIndex.cend())
    {
        m_pGlobal->pLogger->Error("Timestamps::WriteTimestampFinish: No record of timestamp: {}", name);
        return;
    }

    m_pGlobal->vk.vkCmdWriteTimestamp2(
        pCommandBuffer->GetVulkanCommandBuffer().GetVkCommandBuffer(),
        VK_PIPELINE_STAGE_2_NONE,
        m_queryPool.GetVkQueryPool(),
        it->second.index + it->second.span // + span is the reserved position for the finish timestamps(s)
    );
}

std::optional<float> Timestamps::GetTimestampDiffMs(const std::string& name, uint32_t offset) const
{
    const auto it = m_timestampToIndex.find(name);
    if (it == m_timestampToIndex.cend())
    {
        return std::nullopt;
    }

    if (offset > it->second.span)
    {
        m_pGlobal->pLogger->Error("Timestamps::GetTimestampDiffMs: Offset must be < the timestamp span: {}", name);
        return std::nullopt;
    }

    const auto startVal = m_timestampRawData.at(it->second.index + offset);
    const auto finishVal = m_timestampRawData.at(it->second.index + it->second.span + offset);

    // Must be both a start and finish timestamp. Handles cases where a timestamp may have been started, but then
    // never finished due to bailing out (or erroring out) of work
    if (startVal == 0 || finishVal == 0)
    {
        return std::nullopt;
    }

    return float(finishVal - startVal) * (m_timestampPeriod / 1000000.0f);
}

}
