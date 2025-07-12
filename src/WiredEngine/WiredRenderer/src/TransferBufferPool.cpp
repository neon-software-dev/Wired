/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "TransferBufferPool.h"

#include "Global.h"

#include "Wired/GPU/WiredGPU.h"

#include <format>

namespace Wired::Render
{

[[nodiscard]] std::expected<GPU::BufferId, bool> CreateGPUTransferBuffer(Global* pGlobal,
                                                                         GPU::TransferBufferUsageFlags usage,
                                                                         std::size_t byteSize,
                                                                         bool sequentiallyWritten,
                                                                         const std::string& userTag)
{
    const auto transferBufferCreateParams = GPU::TransferBufferCreateParams{
        .usageFlags = usage,
        .byteSize = byteSize,
        .sequentiallyWritten = sequentiallyWritten
    };

    const auto bufferId = pGlobal->pGPU->CreateTransferBuffer(transferBufferCreateParams, userTag);
    if (!bufferId)
    {
        return std::unexpected(false);
    }

    return *bufferId;
}

TransferBufferPool::TransferBufferPool(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

TransferBufferPool::~TransferBufferPool()
{
    m_pGlobal = nullptr;
}

std::expected<GPU::BufferId, bool> TransferBufferPool::Get(const std::string& transferKey,
                                                           GPU::TransferBufferUsageFlags usage,
                                                           const std::size_t& byteSize,
                                                           bool sequentiallyWritten)
{
    const auto it = m_buffers.find(transferKey);

    // If a transfer buffer for the key exists, and it's large enough and has the same usage, then return it
    if (it != m_buffers.cend() &&
        it->second.byteSize >= byteSize &&
        it->second.usage == usage)
    {
        return it->second.gpuTransferBuffer;
    }

    // Otherwise, if the transfer buffer exists, we need to destroy it before we create another
    if (it != m_buffers.cend())
    {
        m_pGlobal->pGPU->DestroyBuffer(it->second.gpuTransferBuffer);
        m_buffers.erase(it);
    }

    // Create a transfer buffer and track it
    const auto newBufferSize = byteSize * 2; // Times two to prevent constant resizes
    const auto bufferId = CreateGPUTransferBuffer(m_pGlobal, usage, newBufferSize, sequentiallyWritten, transferKey);
    if (!bufferId)
    {
        return std::unexpected(false);
    }

    m_buffers.insert({transferKey, TransferBuffer{
        .gpuTransferBuffer = *bufferId,
        .usage = usage,
        .byteSize = newBufferSize}
    });
    return *bufferId;
}

void TransferBufferPool::Destroy()
{
    for (const auto& it : m_buffers)
    {
        m_pGlobal->pGPU->DestroyBuffer(it.second.gpuTransferBuffer);
    }
}

}