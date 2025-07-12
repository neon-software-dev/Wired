/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "UniformBuffers.h"

#include "../Global.h"
#include "../Usages.h"

#include "Buffers.h"

namespace Wired::GPU
{

static constexpr auto ENTRIES_PER_BUFFER = 1024U;

UniformBuffers::UniformBuffers(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

UniformBuffers::~UniformBuffers()
{
    m_pGlobal = nullptr;
}

bool UniformBuffers::Create()
{
    m_pGlobal->pLogger->Info("UniformBuffers: Creating");

    m_entryByteSize = UNIFORM_BUFFER_BYTE_SIZE;

    // Modify entry byte size to be aligned to minUniformBufferOffsetAlignment
    const auto minUniformBufferOffsetAlignment = m_pGlobal->physicalDevice.GetPhysicalDeviceProperties().properties.limits.minUniformBufferOffsetAlignment;
    if (minUniformBufferOffsetAlignment > 0)
    {
        m_entryByteSize = (m_entryByteSize + minUniformBufferOffsetAlignment - 1) & ~(minUniformBufferOffsetAlignment - 1);
    }

    m_bufferByteSize = m_entryByteSize * ENTRIES_PER_BUFFER;

    const auto uniformBuffer = AllocateUniformBuffer();
    if (!uniformBuffer)
    {
        m_pGlobal->pLogger->Error("UniformBuffers::Create: Failed to allocate initial uniform buffer");
        return false;
    }

    m_activeBuffer = *uniformBuffer;

    return true;
}

void UniformBuffers::Destroy()
{
    m_pGlobal->pLogger->Info("UniformBuffers: Destroying");

    if (m_activeBuffer)
    {
        m_pGlobal->pBuffers->DestroyBuffer(m_activeBuffer->bufferId, true);
        m_activeBuffer = std::nullopt;
    }

    for (const auto& bufferId : m_cachedBuffers)
    {
        m_pGlobal->pBuffers->DestroyBuffer(bufferId, true);
    }
    m_cachedBuffers.clear();

    for (const auto& bufferId : m_tappedBuffers)
    {
        m_pGlobal->pBuffers->DestroyBuffer(bufferId, true);
    }
    m_tappedBuffers.clear();
}

std::expected<DynamicUniformBuffer, bool> UniformBuffers::GetFreeUniformBuffer()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    assert(m_activeBuffer);
    if (!m_activeBuffer)
    {
        m_pGlobal->pLogger->Error("UniformBuffers::GetFreeUniformBuffer: No active buffer exists");
        return std::unexpected(false);
    }

    const auto dynamicUniformBuffer = DynamicUniformBuffer{
        .bufferId = m_activeBuffer->bufferId,
        .byteOffset = m_activeBuffer->entryOffset * m_entryByteSize
    };

    m_activeBuffer->entryOffset++;

    if (m_activeBuffer->entryOffset == ENTRIES_PER_BUFFER)
    {
        TapOutActiveBuffer();
    }

    return dynamicUniformBuffer;
}

void UniformBuffers::TapOutActiveBuffer()
{
    m_tappedBuffers.insert(m_activeBuffer->bufferId);
    m_activeBuffer = std::nullopt;

    const auto newActiveBuffer = AllocateUniformBuffer();
    if (!newActiveBuffer)
    {
        m_pGlobal->pLogger->Error("UniformBuffers::TapOutActiveBuffer: Failed to allocate a new active buffer");
        return;
    }

    m_activeBuffer = *newActiveBuffer;
}

std::expected<UniformBuffers::UniformBuffer, bool> UniformBuffers::AllocateUniformBuffer()
{
    //
    // If a cached buffer exists, use it
    //
    if (!m_cachedBuffers.empty())
    {
        BufferId cachedBufferId = *m_cachedBuffers.cbegin();
        m_cachedBuffers.erase(cachedBufferId);

        return UniformBuffer{
            .bufferId = cachedBufferId,
            .entryOffset = 0
        };
    }

    //
    // Otherwise, allocate a new buffer
    //
    m_pGlobal->pLogger->Debug("UniformBuffers: Allocating a new uniform buffer");

    const auto bufferId = m_pGlobal->pBuffers->CreateBuffer(
        {BufferUsageFlag::GraphicsUniformRead},
        m_bufferByteSize,
        false, // TODO Perf: dedicated? Perf seems better (atm) without dedicated
        "Uniform"
    );
    if (!bufferId)
    {
        m_pGlobal->pLogger->Error("UniformBuffers::AllocateUniformBuffer: Buffers system failed to allocate new uniform buffer");
        return std::unexpected(false);
    }

    return UniformBuffer{
        .bufferId = *bufferId,
        .entryOffset = 0
    };
}

void UniformBuffers::RunCleanUp()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    //
    // Move any tapped buffers with no GPU usages into the cached list for re-use
    //
    std::vector<BufferId> buffersToCache;

    for (const auto& tappedBuffer : m_tappedBuffers)
    {
        const auto gpuBuffer = m_pGlobal->pBuffers->GetBuffer(tappedBuffer, false);
        if (!gpuBuffer)
        {
            m_pGlobal->pLogger->Error("UniformBuffers::RunCleanUp: No such buffer exists: {}", tappedBuffer.id);
            continue;
        }

        const bool bufferInUse = m_pGlobal->pUsages->buffers.GetGPUUsageCount(gpuBuffer->vkBuffer) > 0;
        if (!bufferInUse)
        {
            buffersToCache.push_back(tappedBuffer);
        }
    }

    for (const auto& bufferId : buffersToCache)
    {
        m_cachedBuffers.insert(bufferId);
        m_tappedBuffers.erase(bufferId);
    }
}

}
