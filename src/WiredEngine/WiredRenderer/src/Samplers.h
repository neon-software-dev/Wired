/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_SAMPLERS_H
#define WIREDENGINE_WIREDRENDERER_SRC_SAMPLERS_H

#include <Wired/Render/SamplerCommon.h>

#include <Wired/GPU/GPUSamplerCommon.h>
#include <Wired/GPU/GPUId.h>

#include <unordered_map>
#include <optional>

namespace NCommon
{
    class ILogger;
}

namespace Wired::Render
{
    struct Global;

    class Samplers
    {
        public:

            explicit Samplers(Global* pGlobal);
            ~Samplers();

            [[nodiscard]] bool StartUp();
            [[nodiscard]] std::optional<GPU::SamplerId> GetOrCreateSampler(const GPU::SamplerInfo& samplerInfo, const std::string& userTag);
            void ShutDown();

            [[nodiscard]] GPU::SamplerId GetDefaultSampler(const DefaultSampler& defaultSampler) const;

        private:

            [[nodiscard]] bool CreateDefaultSamplers();

            [[nodiscard]] static GPU::SamplerInfo GetDefaultSamplerInfo(const DefaultSampler& sampler);

        private:

            Global* m_pGlobal;

            std::unordered_map<std::size_t, GPU::SamplerId> m_samplers;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_SAMPLERS_H
