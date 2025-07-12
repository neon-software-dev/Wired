/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Effects.h"

#include "../Global.h"
#include "../Pipelines.h"

#include <NEON/Common/Log/ILogger.h>

#include <cstring>

namespace Wired::Render
{

struct alignas(16) ColorCorrectionEffectUniformPayload
{
    // Required
    alignas(4) uint32_t renderWidth{0};
    alignas(4) uint32_t renderHeight{0};

    // Effect-specific

    // Tone Mapping
    alignas(4) uint32_t doToneMapping{0};
    alignas(4) float exposure{1.0f};

    // Gamma Correction
    alignas(4) uint32_t doGammaCorrection{0};
    alignas(4) float gamma{2.2f};
};

std::expected<Effect, bool> ColorCorrectionEffect(Global* pGlobal)
{
    const std::string shaderBaseName = "color_correction.comp";

    const auto computePipelineParams = GPU::ComputePipelineParams{
        .shaderName = pGlobal->pPipelines->GetShaderNameFromBaseName(shaderBaseName)
    };

    const auto computePipelineId = pGlobal->pPipelines->GetOrCreatePipeline(computePipelineParams);
    if (!computePipelineId)
    {
        pGlobal->pLogger->Error("ColorCorrectionEffect: Compute pipeline doesn't exist: {}", shaderBaseName);
        return std::unexpected(false);
    }

    const auto& renderSettings = pGlobal->renderSettings;

    const ColorCorrectionEffectUniformPayload payload{
        .renderWidth = renderSettings.resolution.w,
        .renderHeight = renderSettings.resolution.h,
        .doToneMapping = renderSettings.hdr,
        .exposure = renderSettings.exposure,
        .doGammaCorrection = 1,
        .gamma = renderSettings.gamma
    };

    std::vector<std::byte> payloadBytes(sizeof(ColorCorrectionEffectUniformPayload));
    memcpy(payloadBytes.data(), &payload, sizeof(ColorCorrectionEffectUniformPayload));

    return Effect{
        .userTag = "ColorCorrection",
        .computePipelineId = *computePipelineId,
        .inputSampler = DefaultSampler::NearestClamp,
        .uniformPayloads = {{"u_data", payloadBytes}},
        .samplerBinds = {}
    };
}

struct alignas(16) FXAAEffectUniformPayload
{
    alignas(4) uint32_t renderWidth{0};
    alignas(4) uint32_t renderHeight{0};
};

std::expected<Effect, bool> FXAAEffect(Global* pGlobal)
{
    const std::string shaderBaseName = "FXAA.comp";

    const auto computePipelineParams = GPU::ComputePipelineParams{
        .shaderName = pGlobal->pPipelines->GetShaderNameFromBaseName(shaderBaseName)
    };

    const auto computePipelineId = pGlobal->pPipelines->GetOrCreatePipeline(computePipelineParams);
    if (!computePipelineId)
    {
        pGlobal->pLogger->Error("FXAAEffect: Compute pipeline doesn't exist: {}", shaderBaseName);
        return std::unexpected(false);
    }

    const auto& renderSettings = pGlobal->renderSettings;

    const FXAAEffectUniformPayload payload{
        .renderWidth = renderSettings.resolution.w,
        .renderHeight = renderSettings.resolution.h
    };

    std::vector<std::byte> payloadBytes(sizeof(FXAAEffectUniformPayload));
    memcpy(payloadBytes.data(), &payload, sizeof(FXAAEffectUniformPayload));

    return Effect{
        .userTag = "FXAA",
        .computePipelineId = *computePipelineId,
        .inputSampler = DefaultSampler::LinearClamp,
        .uniformPayloads = {{"u_data", payloadBytes}},
        .samplerBinds = {}
    };
}

struct alignas(16) VolumetricLightingEffectUniformPayload
{
    alignas(4) uint32_t renderWidth{0};
    alignas(4) uint32_t renderHeight{0};

    alignas(16) glm::vec3 camera_worldPos{0};

    alignas(16) glm::mat4 viewTransform{1};
    alignas(16) glm::mat4 projectionTransform{1};
};

std::expected<Effect, bool> VolumetricLightingEffect(Global* pGlobal, const LightState& lightState, const Camera& camera, TextureId cameraDepthBuffer)
{
    const auto worldCameraViewProjection = GetWorldCameraViewProjection(pGlobal->renderSettings, camera);

    const std::string shaderBaseName = "volumetric_lighting.comp";

    const auto computePipelineParams = GPU::ComputePipelineParams{
        .shaderName = pGlobal->pPipelines->GetShaderNameFromBaseName(shaderBaseName)
    };

    const auto computePipelineId = pGlobal->pPipelines->GetOrCreatePipeline(computePipelineParams);
    if (!computePipelineId)
    {
        pGlobal->pLogger->Error("VolumetricLightingEffect: Compute pipeline doesn't exist: {}", shaderBaseName);
        return std::unexpected(false);
    }

    const auto& renderSettings = pGlobal->renderSettings;

    const VolumetricLightingEffectUniformPayload dataPayload{
        .renderWidth = renderSettings.resolution.w,
        .renderHeight = renderSettings.resolution.h,
        .camera_worldPos = camera.position,
        .viewTransform = worldCameraViewProjection->viewTransform,
        .projectionTransform = worldCameraViewProjection->projectionTransform->GetProjectionMatrix()
    };

    std::vector<std::byte> dataPayloadBytes(sizeof(VolumetricLightingEffectUniformPayload));
    memcpy(dataPayloadBytes.data(), &dataPayload, sizeof(VolumetricLightingEffectUniformPayload));

    const auto lightPayload = GetLightPayload(pGlobal->renderSettings, lightState.light);

    std::vector<std::byte> lightPayloadBytes(sizeof(LightPayload));
    memcpy(lightPayloadBytes.data(), &lightPayload, sizeof(LightPayload));

    return Effect{
        .userTag = "VolumetricLighting",
        .computePipelineId = *computePipelineId,
        .inputSampler = DefaultSampler::LinearClamp,
        .uniformPayloads = {
            {"u_data", dataPayloadBytes},
            {"u_lightPayload", lightPayloadBytes}
        },
        .samplerBinds = {
            {"i_cameraDepthBuffer", {cameraDepthBuffer, DefaultSampler::NearestClamp}},
            {"i_shadowSampler_array", {*lightState.shadowMapTextureId, DefaultSampler::NearestClamp}}
        }
    };
}

}
