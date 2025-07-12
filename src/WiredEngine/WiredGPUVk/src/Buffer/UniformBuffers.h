/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_BUFFER_UNIFORMBUFFERS_H
#define WIREDENGINE_WIREDGPUVK_SRC_BUFFER_UNIFORMBUFFERS_H

#include "GPUBuffer.h"

#include <Wired/GPU/GPUId.h>

#include <unordered_set>
#include <mutex>
#include <expected>
#include <optional>

namespace Wired::GPU
    {
    static constexpr std::size_t UNIFORM_BUFFER_BYTE_SIZE = 1024U;

    struct Global;

    struct DynamicUniformBuffer
    {
        BufferId bufferId{};
        std::size_t byteOffset{};
    };

    class UniformBuffers
    {
        public:

            explicit UniformBuffers(Global* pGlobal);
            ~UniformBuffers();

            [[nodiscard]] bool Create();
            void Destroy();

            void RunCleanUp();

            [[nodiscard]] std::expected<DynamicUniformBuffer, bool> GetFreeUniformBuffer();

        private:

            struct UniformBuffer
            {
                BufferId bufferId{};
                std::size_t entryOffset{0};
            };

        private:

            [[nodiscard]] std::expected<UniformBuffer, bool> AllocateUniformBuffer();

            void TapOutActiveBuffer();

        private:

            Global* m_pGlobal{nullptr};

            std::size_t m_entryByteSize{0};
            std::size_t m_bufferByteSize{0};

            std::optional<UniformBuffer> m_activeBuffer;
            std::unordered_set<BufferId> m_cachedBuffers;
            std::unordered_set<BufferId> m_tappedBuffers;
            std::mutex m_mutex;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_BUFFER_UNIFORMBUFFERS_H
