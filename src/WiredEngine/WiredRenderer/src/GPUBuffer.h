/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_GPUBUFFER_H
#define WIREDENGINE_WIREDRENDERER_SRC_GPUBUFFER_H

#include <Wired/Render/BufferCommon.h>

#include <Wired/GPU/GPUCommon.h>

#include <cstddef>
#include <expected>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>

namespace Wired::Render
{
    class TransferBufferPool;
    struct Global;

    /**
     * Internal basic wrapper around a GPU storage buffer.
     *
     * Warning: Alignment requirements of data within the buffer is up to the user to
     * get right. This class simply puts bytes at the places you tell it to, without any
     * thought as to alignment of the data within/across those bytes.
     */
    class GPUBuffer
    {
        public:

            struct DataPush
            {
                Data data{};
            };

            struct DataUpdate
            {
                Data data{};
                std::size_t destByteOffset{0};
            };

        public:

            GPUBuffer() = default;

            /**
             * Create the buffer. If it was already created the previous buffer is
             * destroyed and a new one created.
             */
            [[nodiscard]] bool Create(Global* pGlobal,
                                      const GPU::BufferUsageFlags& usage,
                                      std::size_t byteSize,
                                      bool dedicatedMemory,
                                      const std::string& userTag);
            void Destroy();

            [[nodiscard]] GPU::BufferId GetBufferId() const noexcept { return m_bufferId; }
            [[nodiscard]] std::size_t GetByteSize() const noexcept { return m_byteSize; }

            /**
             * Update one or more portions of the buffer's data
             */
            [[nodiscard]] bool Update(GPU::CopyPass copyPass,
                                      const std::string& transferKey,
                                      const std::vector<DataUpdate>& updates);

            /**
             * Reallocates the buffer to byteSize bytes. Any data previously in the buffer
             * will be transferred to the new buffer.
             *
             * If shrinking the buffer size, any existing data outside of the new bounds
             * will be discarded. If enlarging the buffer size, the newly expanded area
             * is in an undefined state until updated.
             */
            [[nodiscard]] bool ResizeRetaining(GPU::CopyPass copyPass, const std::size_t& byteSize);

            /**
             * Reallocates the buffer to byteSize bytes. Any data previously in the buffer
             * will be discarded. The data in the buffer afterwards is in an undefined
             * state until updated.
             */
            [[nodiscard]] bool ResizeDiscarding(const std::size_t& byteSize);

        private:

            [[nodiscard]] bool Resize(const std::optional<GPU::CopyPass>& copyPass, const std::size_t& byteSize);

        private:

            Global* m_pGlobal{nullptr};
            TransferBufferPool* m_pTransferBufferPool{nullptr};
            GPU::BufferUsageFlags m_usage{};
            std::size_t m_byteSize{0};
            bool m_dedicatedMemory{false};
            std::string m_userTag;
            GPU::BufferId m_bufferId;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_GPUBUFFER_H
