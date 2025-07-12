/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_TRANSFERBUFFERPOOL_H
#define WIREDENGINE_WIREDRENDERER_SRC_TRANSFERBUFFERPOOL_H

#include <Wired/GPU/GPUId.h>
#include <Wired/GPU/GPUCommon.h>

#include <unordered_map>
#include <string>
#include <expected>

namespace Wired::Render
{
    struct Global;

    // TODO Perf: Add an OnFrameFinished call which over time reduces the size of and
    //  deletes transfer buffers, depending on usage patterns.
    class TransferBufferPool
    {
        public:

            explicit TransferBufferPool(Global* pGlobal);
            ~TransferBufferPool();

            [[nodiscard]] std::expected<GPU::BufferId, bool> Get(const std::string& transferKey,
                                                                 GPU::TransferBufferUsageFlags usage,
                                                                 const std::size_t& byteSize,
                                                                 bool sequentiallyWritten);

            void Destroy();

        private:

            struct TransferBuffer
            {
                GPU::BufferId gpuTransferBuffer{};
                GPU::TransferBufferUsageFlags usage{};
                std::size_t byteSize{0};
            };

        private:

            Global* m_pGlobal;

            // Transfer key -> buffer
            std::unordered_map<std::string, TransferBuffer> m_buffers;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_TRANSFERBUFFERPOOL_H
