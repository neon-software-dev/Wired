/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_BUFFER_BUFFERS_H
#define WIREDENGINE_WIREDGPUVK_SRC_BUFFER_BUFFERS_H

#include "GPUBuffer.h"
#include "BufferDef.h"
#include "BufferCommon.h"

#include <Wired/GPU/GPUId.h>
#include <Wired/GPU/GPUCommon.h>

#include <expected>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <optional>

namespace Wired::GPU
{
    struct Global;
    class CommandBuffer;

    class Buffers
    {
        public:

            explicit Buffers(Global* pGlobal);
            ~Buffers();

            void Destroy();
            void RunCleanUp();

            [[nodiscard]] std::expected<BufferId, bool> CreateTransferBuffer(const TransferBufferUsageFlags& transferBufferUsageFlags,
                                                                             const std::size_t& byteSize,
                                                                             bool sequentiallyWritten,
                                                                             const std::string& tag);

            [[nodiscard]] std::expected<BufferId, bool> CreateBuffer(const BufferUsageFlags& bufferUsageFlags,
                                                                     const std::size_t& byteSize,
                                                                     bool dedicatedMemory,
                                                                     const std::string& tag);

            void DestroyBuffer(BufferId bufferId, bool destroyImmediately);

            [[nodiscard]] std::optional<GPUBuffer> GetBuffer(BufferId bufferId, bool cycled);

            [[nodiscard]] std::expected<void*, bool> MapBuffer(BufferId bufferId, bool cycle);
            bool UnmapBuffer(BufferId bufferId);

            bool BarrierBufferRangeForUsage(CommandBuffer* pCommandBuffer, const GPUBuffer& gpuBuffer, const std::size_t& byteOffset, const std::size_t& byteSize, BufferUsageMode destUsageMode);
            bool BarrierBufferRangeToDefaultUsage(CommandBuffer* pCommandBuffer, const GPUBuffer& gpuBuffer, const std::size_t& byteOffset, const std::size_t& byteSize, BufferUsageMode sourceUsageMode);

        private:

            struct Buffer
            {
                BufferId id{};
                std::string tag;

                uint32_t activeBufferIndex{0};
                std::vector<GPUBuffer> gpuBuffers;
            };

        private:

            [[nodiscard]] std::expected<GPUBuffer, bool> CreateGPUBuffer(const BufferDef& bufferDef, const std::string& tag);

            [[nodiscard]] bool CycleBufferIfNeeded(Buffer& buffer);

            void CleanUp_DeletedBuffers();
            void CleanUp_UnusedBuffers();

            void DestroyBufferObjects(const Buffer& buffer);
            void DestroyGPUBufferObjects(const GPUBuffer& gpuBuffer);

        private:

            Global* m_pGlobal;

            std::unordered_map<BufferId, Buffer> m_buffers;
            std::unordered_set<BufferId> m_buffersMarkedForDeletion;
            mutable std::recursive_mutex m_buffersMutex;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_BUFFER_BUFFERS_H
