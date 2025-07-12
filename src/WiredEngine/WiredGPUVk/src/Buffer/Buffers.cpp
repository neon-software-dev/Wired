/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Buffers.h"
#include "BufferAllocation.h"

#include "../Global.h"
#include "../Usages.h"

#include "../State/CommandBuffer.h"
#include "../Vulkan/VulkanDebugUtil.h"

#include <NEON/Common/Log/ILogger.h>

#include <algorithm>

namespace Wired::GPU
{

Buffers::Buffers(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

Buffers::~Buffers()
{
    m_pGlobal = nullptr;
}

void Buffers::Destroy()
{
    m_pGlobal->pLogger->Info("Buffers: Destroying");

    while (!m_buffers.empty())
    {
        DestroyBuffer(m_buffers.cbegin()->first, true);
    }
}

void Buffers::RunCleanUp()
{
    // Clean up buffers that are marked as deleted which no longer have any references/usages
    CleanUp_DeletedBuffers();

    // Clean up buffers which aren't used; try to collapse buffers back to one GPU buffers
    CleanUp_UnusedBuffers();
}

void Buffers::CleanUp_DeletedBuffers()
{
    std::lock_guard<std::recursive_mutex> lock(m_buffersMutex);

    std::unordered_set<BufferId> noLongerMarkedForDeletion;

    for (const auto& bufferId : m_buffersMarkedForDeletion)
    {
        const auto buffer = m_buffers.find(bufferId);
        if (buffer == m_buffers.cend())
        {
            m_pGlobal->pLogger->Error("Buffers::RunCleanUp: Buffer marked for deletion doesn't exist: {}", bufferId.id);
            noLongerMarkedForDeletion.insert(bufferId);
            continue;
        }

        // To destroy the buffer, all of its gpuBuffers have to both be unused by any command buffer and no system
        // exists with a lock on it
        const bool allGPUBuffersUnused = std::ranges::all_of(buffer->second.gpuBuffers, [this](const auto& gpuBuffer){
            const bool noUsages = m_pGlobal->pUsages->buffers.GetGPUUsageCount(gpuBuffer.vkBuffer) == 0;
            const bool noLocks = m_pGlobal->pUsages->buffers.GetLockCount(gpuBuffer.vkBuffer) == 0;

            return noUsages && noLocks;
        });

        if (allGPUBuffersUnused)
        {
            DestroyBuffer(bufferId, true);
            noLongerMarkedForDeletion.insert(bufferId);
        }
    }

    for (const auto& bufferId : noLongerMarkedForDeletion)
    {
        m_buffersMarkedForDeletion.erase(bufferId);
    }
}

void Buffers::CleanUp_UnusedBuffers()
{
    // TODO Perf
}

std::expected<BufferId, bool> Buffers::CreateTransferBuffer(const TransferBufferUsageFlags& transferBufferUsageFlags,
                                                            const std::size_t& byteSize,
                                                            bool sequentiallyWritten,
                                                            const std::string& tag)
{
    //
    // Determine default usage mode. Order matters in here.
    //
    BufferUsageMode defaultUsageMode{};

    if (transferBufferUsageFlags.contains(TransferBufferUsageFlag::Upload))
    {
        defaultUsageMode = BufferUsageMode::TransferSrc;
    }
    else if (transferBufferUsageFlags.contains(TransferBufferUsageFlag::Download))
    {
        defaultUsageMode = BufferUsageMode::TransferDst;
    }
    else
    {
        m_pGlobal->pLogger->Error("Buffers::CreateTransferBuffer: Unsupported usage flags");
        return std::unexpected(false);
    }

    //
    // Determine Vk flags
    //
    VkBufferUsageFlags vkBufferUsageFlags{};
    if (transferBufferUsageFlags.contains(TransferBufferUsageFlag::Upload))
    {
        vkBufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (transferBufferUsageFlags.contains(TransferBufferUsageFlag::Download))
    {
        vkBufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    VmaAllocationCreateFlags vmaAllocationCreateFlags{};
    if (sequentiallyWritten)
    {
        vmaAllocationCreateFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }
    else
    {
        vmaAllocationCreateFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    }

    const BufferDef bufferDef{
        .isTransferBuffer = true,
        .defaultUsageMode = defaultUsageMode,
        .byteSize = byteSize,
        .vkBufferUsageFlags = vkBufferUsageFlags,
        .vmaMemoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
        .vmaAllocationCreateFlags = vmaAllocationCreateFlags
    };

    const auto gpuBuffer = CreateGPUBuffer(bufferDef, tag);
    if (!gpuBuffer)
    {
        m_pGlobal->pLogger->Error("Buffers::CreateBuffer: Call to CreateGPUBuffer() failed for: {}", tag);
        return std::unexpected(false);
    }

    //
    // Record results
    //
    std::lock_guard<std::recursive_mutex> lock(m_buffersMutex);

    Buffer buffer{};
    buffer.id = m_pGlobal->ids.bufferIds.GetId();
    buffer.tag = tag;
    buffer.activeBufferIndex = 0;
    buffer.gpuBuffers.push_back(*gpuBuffer);

    m_buffers.insert({buffer.id, buffer});

    return buffer.id;
}

std::expected<BufferId, bool> Buffers::CreateBuffer(const BufferUsageFlags& bufferUsageFlags,
                                                    const size_t& byteSize,
                                                    bool dedicatedMemory,
                                                    const std::string& tag)
{
    //
    // Determine default usage mode. Order matters here.
    //
    BufferUsageMode defaultUsageMode{};

    if (bufferUsageFlags.contains(BufferUsageFlag::Vertex))
    {
        defaultUsageMode = BufferUsageMode::VertexRead;
    }
    else if (bufferUsageFlags.contains(BufferUsageFlag::Index))
    {
        defaultUsageMode = BufferUsageMode::IndexRead;
    }
    else if (bufferUsageFlags.contains(BufferUsageFlag::Indirect))
    {
        defaultUsageMode = BufferUsageMode::Indirect;
    }
    else if (bufferUsageFlags.contains(BufferUsageFlag::GraphicsUniformRead))
    {
        defaultUsageMode = BufferUsageMode::GraphicsUniformRead;
    }
    else if (bufferUsageFlags.contains(BufferUsageFlag::GraphicsStorageRead))
    {
        defaultUsageMode = BufferUsageMode::GraphicsStorageRead;
    }
    else if (bufferUsageFlags.contains(BufferUsageFlag::ComputeUniformRead))
    {
        defaultUsageMode = BufferUsageMode::ComputeUniformRead;
    }
    else if (bufferUsageFlags.contains(BufferUsageFlag::ComputeStorageRead) ||
             bufferUsageFlags.contains(BufferUsageFlag::ComputeStorageReadWrite))
    {
        // Note: Defaulting both to read, not readwrite. Fixes scenario with a buffer
        // only used as compute read/write and used by two consecutive compute dispatches.
        defaultUsageMode = BufferUsageMode::ComputeStorageRead;
    }
    else if (bufferUsageFlags.contains(BufferUsageFlag::TransferSrc))
    {
        defaultUsageMode = BufferUsageMode::TransferSrc;
    }
    else
    {
        m_pGlobal->pLogger->Error("Buffers::CreateBuffer: Unsupported usage flags");
        return std::unexpected(false);
    }

    //
    // Determine Vk flags
    //
    VkBufferUsageFlags vkBufferUsageFlags{};
    VmaAllocationCreateFlags vmaAllocationCreateFlags{};

    if (bufferUsageFlags.contains(BufferUsageFlag::Vertex))
    {
        vkBufferUsageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (bufferUsageFlags.contains(BufferUsageFlag::Index))
    {
        vkBufferUsageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (bufferUsageFlags.contains(BufferUsageFlag::Indirect))
    {
        vkBufferUsageFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }
    if (bufferUsageFlags.contains(BufferUsageFlag::GraphicsUniformRead))
    {
        vkBufferUsageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        vmaAllocationCreateFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    }
    if (bufferUsageFlags.contains(BufferUsageFlag::GraphicsStorageRead))
    {
        vkBufferUsageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (bufferUsageFlags.contains(BufferUsageFlag::ComputeUniformRead))
    {
        vkBufferUsageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        vmaAllocationCreateFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    }
    if (bufferUsageFlags.contains(BufferUsageFlag::ComputeStorageRead))
    {
        vkBufferUsageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (bufferUsageFlags.contains(BufferUsageFlag::ComputeStorageReadWrite))
    {
        vkBufferUsageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (bufferUsageFlags.contains(BufferUsageFlag::TransferSrc))
    {
        vkBufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (bufferUsageFlags.contains(BufferUsageFlag::TransferDst))
    {
        vkBufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    if (dedicatedMemory)
    {
        vmaAllocationCreateFlags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    }

    const BufferDef bufferDef{
        .isTransferBuffer = false,
        .defaultUsageMode = defaultUsageMode,
        .byteSize = byteSize,
        .vkBufferUsageFlags = vkBufferUsageFlags,
        .vmaMemoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .vmaAllocationCreateFlags = vmaAllocationCreateFlags
    };

    const auto gpuBuffer = CreateGPUBuffer(bufferDef, tag);
    if (!gpuBuffer)
    {
        m_pGlobal->pLogger->Error("Buffers::CreateBuffer: Call to CreateGPUBuffer() failed for: {}", tag);
        return std::unexpected(false);
    }

    //
    // Record results
    //
    std::lock_guard<std::recursive_mutex> lock(m_buffersMutex);

    Buffer buffer{};
    buffer.id = m_pGlobal->ids.bufferIds.GetId();
    buffer.tag = tag;
    buffer.activeBufferIndex = 0;
    buffer.gpuBuffers.push_back(*gpuBuffer);

    m_buffers.insert({buffer.id, buffer});

    return buffer.id;
}

std::expected<GPUBuffer, bool> Buffers::CreateGPUBuffer(const BufferDef& bufferDef, const std::string& tag)
{
    if (bufferDef.byteSize == 0)
    {
        m_pGlobal->pLogger->Error("Buffers::CreateBuffer: Tried to create a zero-sized buffer for: {}", tag);
        return std::unexpected(false);
    }

    //
    // Create a VMA allocation for the buffer
    //
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferDef.byteSize;
    bufferInfo.usage = bufferDef.vkBufferUsageFlags;

    assert(bufferInfo.size != 0);

    VmaAllocationCreateInfo vmaAllocCreateInfo{};
    vmaAllocCreateInfo.usage = bufferDef.vmaMemoryUsage;
    vmaAllocCreateInfo.flags = bufferDef.vmaAllocationCreateFlags;

    BufferAllocation bufferAllocation{};
    bufferAllocation.vmaAllocationCreateInfo = vmaAllocCreateInfo;

    VkBuffer vkBuffer{VK_NULL_HANDLE};

    const auto result = vmaCreateBuffer(
        m_pGlobal->vma,
        &bufferInfo,
        &vmaAllocCreateInfo,
        &vkBuffer,
        &bufferAllocation.vmaAllocation,
        &bufferAllocation.vmaAllocationInfo);
    if (result != VK_SUCCESS)
    {
        m_pGlobal->pLogger->Error("CreateBuffer: vmaCreateBuffer call failure, result code: {}", (uint32_t)result);
        return std::unexpected(false);
    }

    SetDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_BUFFER, (uint64_t)vkBuffer, std::format("Buffer-{}", tag));

    return GPUBuffer{
        .vkBuffer = vkBuffer,
        .bufferDef = bufferDef,
        .bufferAllocation = bufferAllocation
    };
}

void Buffers::DestroyBuffer(BufferId bufferId, bool destroyImmediately)
{
    std::lock_guard<std::recursive_mutex> lock(m_buffersMutex);

    const auto it = m_buffers.find(bufferId);
    if (it == m_buffers.cend())
    {
        m_pGlobal->pLogger->Warning("Buffers::DestroyBuffer: No such buffer exists: {}", bufferId.id);
        return;
    }

    if (destroyImmediately)
    {
        DestroyBufferObjects(it->second);

        m_buffers.erase(bufferId);
        m_pGlobal->ids.bufferIds.ReturnId(bufferId);
    }
    else
    {
        m_buffersMarkedForDeletion.insert(bufferId);
    }
}

std::optional<GPUBuffer> Buffers::GetBuffer(BufferId bufferId, bool cycled)
{
    std::lock_guard<std::recursive_mutex> lock(m_buffersMutex);

    auto it = m_buffers.find(bufferId);
    if (it == m_buffers.cend())
    {
        return std::nullopt;
    }

    if (m_buffersMarkedForDeletion.contains(bufferId))
    {
        m_pGlobal->pLogger->Warning("Buffers::GetBuffer: Buffer was marked for deletion, not returning it: {}", bufferId.id);
        return std::nullopt;
    }

    if (cycled)
    {
        if (!CycleBufferIfNeeded(it->second))
        {
            m_pGlobal->pLogger->Error("Buffers::GetBuffer: Failed to cycle the buffer");
            return std::nullopt;
        }
    }

    return it->second.gpuBuffers.at(it->second.activeBufferIndex);
}

bool Buffers::CycleBufferIfNeeded(Buffer& buffer)
{
    // If the active GPU buffer is unused, return it
    if (m_pGlobal->pUsages->buffers.GetGPUUsageCount(buffer.gpuBuffers.at(buffer.activeBufferIndex).vkBuffer) == 0)
    {
        return true;
    }

    // Otherwise, try to find an existing GPU buffer which is unused
    for (uint32_t x = 0; x < buffer.gpuBuffers.size(); ++x)
    {
        // Already tested the active buffer above
        if (x == buffer.activeBufferIndex) { continue; }

        if (m_pGlobal->pUsages->buffers.GetGPUUsageCount(buffer.gpuBuffers.at(x).vkBuffer) == 0)
        {
            buffer.activeBufferIndex = x;
            return true;
        }
    }

    // Otherwise, create a new GPU buffer
    const auto copyGPUBuffer = buffer.gpuBuffers.at(0);

    const auto gpuBuffer = CreateGPUBuffer(copyGPUBuffer.bufferDef, buffer.tag);
    if (!gpuBuffer)
    {
        m_pGlobal->pLogger->Error("Buffers::CycleBufferIfNeeded: Failed to create new buffer for cycling");
        return false;
    }

    buffer.gpuBuffers.push_back(*gpuBuffer);
    buffer.activeBufferIndex = (uint32_t)buffer.gpuBuffers.size() - 1;

    return true;
}

std::expected<void*, bool> Buffers::MapBuffer(BufferId bufferId, bool cycle)
{
    const auto gpuBuffer = GetBuffer(bufferId, cycle);
    if (!gpuBuffer)
    {
        m_pGlobal->pLogger->Error("Buffers::MapBuffer: Failed to get or cycle buffer: {}", bufferId.id);
        return std::unexpected(false);
    }

    const bool isRandomAccessAlloc = gpuBuffer->bufferDef.vmaAllocationCreateFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    const bool isSequentialAccessAlloc = gpuBuffer->bufferDef.vmaAllocationCreateFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (!isRandomAccessAlloc && !isSequentialAccessAlloc)
    {
        m_pGlobal->pLogger->Error("Buffers::MapBuffer: Buffer is not a mappable buffer: {}", bufferId.id);
        return std::unexpected(false);
    }

    void *pMappedBuffer{nullptr};
    const auto result = vmaMapMemory(m_pGlobal->vma, gpuBuffer->bufferAllocation.vmaAllocation, &pMappedBuffer);
    if (result != VK_SUCCESS)
    {
        m_pGlobal->pLogger->Error("Buffers::MapBuffer: Call to vmaMapMemory() failed, error code: {}", (uint32_t)result);
        return std::unexpected(false);
    }

    return pMappedBuffer;
}

bool Buffers::UnmapBuffer(BufferId bufferId)
{
    const auto gpuBuffer = GetBuffer(bufferId, false);
    if (!gpuBuffer)
    {
        m_pGlobal->pLogger->Error("Buffers::UnmapBuffer: Failed to get buffer: {}", bufferId.id);
        return false;
    }

    vmaUnmapMemory(m_pGlobal->vma, gpuBuffer->bufferAllocation.vmaAllocation);

    return true;
}

bool Buffers::BarrierBufferRangeForUsage(CommandBuffer* pCommandBuffer,
                                         const GPUBuffer& gpuBuffer,
                                         const std::size_t& byteOffset,
                                         const std::size_t& byteSize,
                                         BufferUsageMode destUsageMode)
{
    pCommandBuffer->CmdBufferPipelineBarrier(gpuBuffer, byteOffset, byteSize, gpuBuffer.bufferDef.defaultUsageMode, destUsageMode);

    return true;
}

bool Buffers::BarrierBufferRangeToDefaultUsage(CommandBuffer* pCommandBuffer,
                                               const GPUBuffer& gpuBuffer,
                                               const std::size_t& byteOffset,
                                               const std::size_t& byteSize,
                                               BufferUsageMode sourceUsageMode)
{
    pCommandBuffer->CmdBufferPipelineBarrier(gpuBuffer, byteOffset, byteSize, sourceUsageMode, gpuBuffer.bufferDef.defaultUsageMode);

    return true;
}

void Buffers::DestroyBufferObjects(const Buffers::Buffer& buffer)
{
    m_pGlobal->pLogger->Debug("Buffers: Destroying buffer objects: {}", buffer.id.id);

    for (const auto& gpuBuffer : buffer.gpuBuffers)
    {
        DestroyGPUBufferObjects(gpuBuffer);
    }
}

void Buffers::DestroyGPUBufferObjects(const GPUBuffer& gpuBuffer)
{
    RemoveDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_BUFFER, (uint64_t)gpuBuffer.vkBuffer);
    vmaDestroyBuffer(m_pGlobal->vma, gpuBuffer.vkBuffer, gpuBuffer.bufferAllocation.vmaAllocation);
}

}
