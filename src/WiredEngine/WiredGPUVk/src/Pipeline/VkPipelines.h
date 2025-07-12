/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_PIPELINE_VKPIPELINES_H
#define WIREDENGINE_WIREDGPUVK_SRC_PIPELINE_VKPIPELINES_H

#include <Wired/GPU/GPUId.h>

#include "../Vulkan/VulkanPipeline.h"

#include <unordered_map>
#include <cstdint>
#include <expected>
#include <optional>
#include <mutex>
#include <unordered_set>

namespace Wired::GPU
{
    struct Global;

    class VkPipelines
    {
        public:

            explicit VkPipelines(Global* pGlobal);
            ~VkPipelines();

            void Destroy();

            [[nodiscard]] std::expected<PipelineId, bool> CreateGraphicsPipeline(const VkGraphicsPipelineConfig& graphicsPipelineConfig);
            [[nodiscard]] std::expected<PipelineId, bool> CreateComputePipeline(const VkComputePipelineConfig& computePipelineConfig);

            [[nodiscard]] std::optional<VulkanPipeline> GetPipeline(PipelineId pipelineId) const;

            void DestroyPipeline(PipelineId pipelineId, bool destroyImmediately);

            void RunCleanUp();

        private:

            void CleanUp_DeletedPipelines();

        private:

            Global* m_pGlobal;

            std::unordered_map<PipelineId, VulkanPipeline> m_pipelines;
            mutable std::recursive_mutex m_pipelinesMutex;

            std::unordered_set<PipelineId> m_pipelinesMarkedForDeletion;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_PIPELINE_VKPIPELINES_H
