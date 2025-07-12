/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_TIMESTAMPS_H
#define WIREDENGINE_WIREDGPUVK_SRC_TIMESTAMPS_H

#include "Vulkan/VulkanQueryPool.h"

#include <vector>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace Wired::GPU
{
    struct Global;
    class CommandBuffer;

    class Timestamps
    {
        public:

            [[nodiscard]] static bool QueueFamilySupportsTimestampQueries(Global* pGlobal, uint32_t queueFamilyIndex);
            [[nodiscard]] static std::expected<std::unique_ptr<Timestamps>, bool> Create(Global* pGlobal, const std::string& tag);

        public:

            Timestamps() = default;
            Timestamps(Global* pGlobal, const VulkanQueryPool& queryPool);

            void Destroy();

            void SyncDownTimestamps();
            void ResetForRecording(CommandBuffer* pCommandBuffer);

            void WriteTimestampStart(CommandBuffer* pCommandBuffer, const std::string& name, uint32_t timestampSpan = 1);
            void WriteTimestampFinish(CommandBuffer* pCommandBuffer, const std::string& name);

            [[nodiscard]] std::optional<float> GetTimestampDiffMs(const std::string& name, uint32_t offset = 0) const;

        private:

            struct TimestampTracking
            {
                uint32_t index{0};
                uint32_t span{1};
            };

        private:

            void QueryWrittenTimestamps();
            void ResetQueryPool(CommandBuffer* pCommandBuffer);

        private:

            Global* m_pGlobal{nullptr};
            VulkanQueryPool m_queryPool;
            bool m_initialResetDone{false};

            float m_timestampPeriod{0.0f};

            uint32_t m_freeIndex{0};
            std::unordered_map<std::string, TimestampTracking> m_timestampToIndex;

            std::vector<uint64_t> m_timestampRawData;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_TIMESTAMPS_H
