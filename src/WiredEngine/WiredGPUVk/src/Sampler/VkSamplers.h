/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_SAMPLER_SAMPLERS_H
#define WIREDENGINE_WIREDGPUVK_SRC_SAMPLER_SAMPLERS_H

#include "../Vulkan/VulkanSampler.h"

#include <Wired/GPU/GPUId.h>

#include <Wired/GPU/GPUSamplerCommon.h>

#include <expected>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace Wired::GPU
{
    struct Global;

    class VkSamplers
    {
        public:

            explicit VkSamplers(Global* pGlobal);
            ~VkSamplers();

            void Destroy();

            void RunCleanUp();

            [[nodiscard]] std::expected<SamplerId, bool> CreateSampler(const SamplerInfo& samplerInfo, const std::string& tag);

            void DestroySampler(SamplerId samplerId, bool destroyImmediately);

            [[nodiscard]] std::optional<VulkanSampler> GetSampler(SamplerId samplerId);

        private:

            void CleanUp_DeletedSamplers();

        private:

            Global* m_pGlobal;

            std::unordered_map<SamplerId, VulkanSampler> m_samplers;
            std::unordered_set<SamplerId> m_samplersMarkedForDeletion;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_SAMPLER_SAMPLERS_H
