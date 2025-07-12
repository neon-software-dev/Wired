/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_PIPELINES_H
#define WIREDENGINE_WIREDRENDERER_SRC_PIPELINES_H

#include <Wired/GPU/GPUId.h>
#include <Wired/GPU/GraphicsPipelineParams.h>
#include <Wired/GPU/ComputePipelineParams.h>

#include <expected>
#include <unordered_map>
#include <cstdint>

namespace Wired::Render
{
    struct Global;

    class Pipelines
    {
        public:

            explicit Pipelines(Global* pGlobal);
            ~Pipelines();

            void ShutDown();

            [[nodiscard]] std::expected<GPU::PipelineId, bool> GetOrCreatePipeline(const GPU::GraphicsPipelineParams& graphicsPipelineParams);
            [[nodiscard]] std::expected<GPU::PipelineId, bool> GetOrCreatePipeline(const GPU::ComputePipelineParams& computePipelineParams);

            // Appends the extension of the shader binary type the renderer was started for to the provided base name
            // (e.g. vert.frag -> vert.frag.spv)
            [[nodiscard]] std::string GetShaderNameFromBaseName(const std::string& shaderBaseName) const;

        private:

            using ParamsHash = std::size_t;

        private:

            Global* m_pGlobal;

            std::unordered_map<ParamsHash, GPU::PipelineId> m_pipelines;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_PIPELINES_H
