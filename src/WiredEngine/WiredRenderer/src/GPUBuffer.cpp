/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "GPUBuffer.h"
#include "TransferBufferPool.h"
#include "Global.h"

#include "Wired/GPU/WiredGPU.h"

#include <cassert>
#include <format>
#include <cstring>

namespace Wired::Render
{

std::expected<GPU::BufferId, bool> CreateGPUBuffer(Global* pGlobal,
                                                   GPU::BufferUsageFlags usage,
                                                   std::size_t byteSize,
                                                   bool dedicatedMemory,
                                                   const std::string& userTag)
{
    if (byteSize == 0) { return std::unexpected(false); }

    const auto bufferCreateParams = GPU::BufferCreateParams{
        .usageFlags = std::move(usage),
        .byteSize = byteSize,
        .dedicatedMemory = dedicatedMemory
    };

    const auto bufferId = pGlobal->pGPU->CreateBuffer(bufferCreateParams, userTag);
    if (!bufferId)
    {
        return std::unexpected(false);
    }

    return *bufferId;
}

bool GPUBuffer::Create(Global* pGlobal,
                       const GPU::BufferUsageFlags& usage,
                       std::size_t byteSize,
                       bool dedicatedMemory,
                       const std::string& userTag)
{
    Destroy();

    auto realUsage = usage;
    realUsage.insert(GPU::BufferUsageFlag::TransferSrc);
    realUsage.insert(GPU::BufferUsageFlag::TransferDst);

    if (pGlobal == nullptr)
    {
        int x = 0;
        x=1;
        (void)x;
    }

    const auto bufferId = CreateGPUBuffer(pGlobal, realUsage, byteSize, dedicatedMemory, userTag);
    if (!bufferId)
    {
        return false;
    }

    m_pGlobal = pGlobal;
    m_usage = realUsage;
    m_byteSize = byteSize;
    m_dedicatedMemory = dedicatedMemory;
    m_userTag = userTag;

    m_bufferId = *bufferId;

    return true;
}

void GPUBuffer::Destroy()
{
    if (m_bufferId.IsValid())
    {
        m_pGlobal->pGPU->DestroyBuffer(m_bufferId);
        m_bufferId = {};
    }

    m_usage = {};
    m_byteSize = 0;
}

bool GPUBuffer::Update(GPU::CopyPass copyPass,
                       const std::string& transferKey,
                       const std::vector<DataUpdate>& updates)
{
    assert(m_bufferId.IsValid());
    if (!m_bufferId.IsValid()) { return false; }

    if (updates.empty()) { return true; }

    //
    // Compute total byte size of data to be updated
    //
    std::size_t updatesTotalByteSize = 0;

    for (const auto& update : updates)
    {
        // Validate the update is within bounds of the buffer's data. Note that we don't
        // allow updating unused capacity, only existing data.
        if (update.destByteOffset + update.data.byteSize > m_byteSize) { return false; }

        updatesTotalByteSize += update.data.byteSize;
    }

    //
    // Fetch a transfer buffer for uploading the new data
    //
    const auto transferBuffer = m_pGlobal->pTransferBufferPool->Get(transferKey, {GPU::TransferBufferUsageFlag::Upload}, updatesTotalByteSize, true);
    if (!transferBuffer)
    {
        return false;
    }

    //
    // Fill the transfer buffer with data
    //
    auto mappedTransferData = m_pGlobal->pGPU->MapBuffer(*transferBuffer, true  /* do cycle */);
    std::size_t bytesWritten = 0;

    for (const auto& update : updates)
    {
        memcpy((std::byte*)(*mappedTransferData) + bytesWritten, update.data.pData, update.data.byteSize);
        bytesWritten += update.data.byteSize;
    }
    m_pGlobal->pGPU->UnmapBuffer(*transferBuffer);

    //
    // Transfer the new data to the buffer
    //
    std::size_t transferBufferOffset = 0;

    for (const auto& update : updates)
    {
        m_pGlobal->pGPU->CmdUploadDataToBuffer(
            copyPass,
            *transferBuffer,
            (uint32_t)transferBufferOffset,
            m_bufferId,
            update.destByteOffset,
            update.data.byteSize,
            false /* no cycle */
        );

        transferBufferOffset += update.data.byteSize;
    }

    return true;
}

bool GPUBuffer::ResizeRetaining(GPU::CopyPass copyPass, const std::size_t& byteSize)
{
    return Resize(copyPass, byteSize);
}

bool GPUBuffer::ResizeDiscarding(const std::size_t& byteSize)
{
    return Resize(std::nullopt, byteSize);
}

bool GPUBuffer::Resize(const std::optional<GPU::CopyPass>& copyPass, const std::size_t& byteSize)
{
    // Can't make a zero byte buffer allocation
    if (byteSize == 0) { return false; }

    // Early optimization
    if (byteSize == m_byteSize) { return true; }

    //
    // Create a newly sized GPU buffer
    //
    const auto newBufferId = CreateGPUBuffer(m_pGlobal, m_usage, byteSize, m_dedicatedMemory, m_userTag);
    if (!newBufferId)
    {
        return false;
    }

    //
    // Copy data from the previous buffer to the new buffer, if a copy pass was
    // provided and there was any data in the previous buffer
    //
    if (copyPass && m_byteSize > 0)
    {
        const auto bytesToCopy = std::min(m_byteSize, byteSize);

        m_pGlobal->pGPU->CmdCopyBufferToBuffer(
            *copyPass,
            m_bufferId,
            0,
            *newBufferId,
            0,
            bytesToCopy,
            false /* no cycle */
        );
    }

    //
    // Free the previous buffer
    //
    m_pGlobal->pGPU->DestroyBuffer(m_bufferId);

    //
    // Update state
    //
    m_bufferId = *newBufferId;
    m_byteSize = byteSize;

    return true;
}

}
