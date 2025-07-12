/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_RENDERER_EFFECTS_H
#define WIREDENGINE_WIREDRENDERER_SRC_RENDERER_EFFECTS_H

#include "../Group.h"
#include "../Util/ViewProjection.h"

#include <Wired/Render/Id.h>
#include <Wired/Render/SamplerCommon.h>

#include <Wired/GPU/GPUId.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <expected>

namespace Wired::Render
{
    struct Global;

    struct Effect
    {
        std::string userTag;
        GPU::PipelineId computePipelineId{};
        DefaultSampler inputSampler{DefaultSampler::NearestClamp};
        // Bind point name -> Uniform bytes
        std::unordered_map<std::string, std::vector<std::byte>> uniformPayloads;
        // Bind point name -> Texture+Sampler to be bound
        std::unordered_map<std::string, std::pair<TextureId, DefaultSampler>> samplerBinds;
    };

    [[nodiscard]] std::expected<Effect, bool> ColorCorrectionEffect(Global* pGlobal);
    [[nodiscard]] std::expected<Effect, bool> FXAAEffect(Global* pGlobal);
    [[nodiscard]] std::expected<Effect, bool> VolumetricLightingEffect(Global* pGlobal, const LightState& lightState, const Camera& camera, TextureId cameraDepthBuffer);
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_RENDERER_EFFECTS_H
