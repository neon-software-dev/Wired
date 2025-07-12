/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "EffectRenderer.h"

#include "../Global.h"
#include "../Samplers.h"

#include "Wired/GPU/WiredGPU.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::Render
{

EffectRenderer::EffectRenderer(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

EffectRenderer::~EffectRenderer()
{
    m_pGlobal = nullptr;
}

bool EffectRenderer::StartUp()
{
    m_pGlobal->pLogger->Info("EffectRenderer: Starting Up");

    if (!CreateEffectWorkTexture())
    {
        m_pGlobal->pLogger->Fatal("EffectRenderer::StartUp: Failed to create effect work texture");
        return false;
    }

    return true;
}

void EffectRenderer::ShutDown()
{
    m_pGlobal->pLogger->Info("EffectRenderer: Shutting Down");

    DestroyEffectWorkTexture();
}


void EffectRenderer::OnRenderSettingsChanged()
{
    // Re-create the work texture to be render settings resolution sized
    (void)CreateEffectWorkTexture();
}

bool EffectRenderer::CreateEffectWorkTexture()
{
    const auto commandBufferId = m_pGlobal->pGPU->AcquireCommandBuffer(true, "CreateEffectWorkTexture");
    if (!commandBufferId)
    {
        m_pGlobal->pLogger->Error("EffectRenderer::CreateEffectWorkTexture: Failed to acquire a command buffer");
        return false;
    }

    // Destroy any existing work texture
    DestroyEffectWorkTexture();

    // Create work texture
    const auto createParams = TextureCreateParams{
        .textureType = TextureType::Texture2D,
        .usageFlags = {TextureUsageFlag::PostProcess, TextureUsageFlag::ComputeStorageReadWrite, TextureUsageFlag::TransferSrc},
        .size = {m_pGlobal->renderSettings.resolution.w, m_pGlobal->renderSettings.resolution.h, 1},
        .numLayers = 1,
        .numMipLevels = 1
    };

    const auto result = m_pGlobal->pTextures->CreateFromParams(*commandBufferId, createParams, "EffectWork");
    if (!result)
    {
        m_pGlobal->pLogger->Error("EffectRenderer::CreateEffectWorkTexture: Texture create failed");
        m_pGlobal->pGPU->CancelCommandBuffer(*commandBufferId);
        return false;
    }

    (void)m_pGlobal->pGPU->SubmitCommandBuffer(*commandBufferId);

    m_effectWorkTextureId = *result;

    return true;
}

void EffectRenderer::DestroyEffectWorkTexture()
{
    if (m_effectWorkTextureId.IsValid())
    {
        m_pGlobal->pTextures->DestroyTexture(m_effectWorkTextureId);
        m_effectWorkTextureId = {};
    }
}

void EffectRenderer::RunEffect(GPU::CommandBufferId commandBufferId, const Effect& effect, TextureId inputTextureId)
{
    //
    // Fetch data
    //
    const auto inputTexture = m_pGlobal->pTextures->GetTexture(inputTextureId);
    if (!inputTexture)
    {
        m_pGlobal->pLogger->Error("EffectRenderer::RunEffect: No such input texture exists: {}", inputTextureId.id);
    }

    const auto workTexture = m_pGlobal->pTextures->GetTexture(m_effectWorkTextureId);
    if (!workTexture)
    {
        m_pGlobal->pLogger->Error("EffectRenderer::RunEffect: No such work texture exists: {}", m_effectWorkTextureId.id);
    }

    const auto samplerId = m_pGlobal->pSamplers->GetDefaultSampler(effect.inputSampler);

    //
    // Execute effect work
    //
    const auto computePass = m_pGlobal->pGPU->BeginComputePass(commandBufferId, std::format("RunEffect-{}", effect.userTag));

    m_pGlobal->pGPU->CmdBindPipeline(*computePass, effect.computePipelineId);

    m_pGlobal->pGPU->CmdBindImageViewSampler(*computePass, "i_inputImage", 0, inputTexture->imageId, samplerId);
    m_pGlobal->pGPU->CmdBindStorageReadWriteImage(*computePass, "o_outputImage", workTexture->imageId);

    for (const auto& it: effect.samplerBinds)
    {
        const auto texture = m_pGlobal->pTextures->GetTexture(it.second.first);
        if (!texture)
        {
            m_pGlobal->pLogger->Error("EffectRenderer::RunEffect: No such sampler texture exists: {}", it.second.first.id);
            continue;
        }

        const auto sampler = m_pGlobal->pSamplers->GetDefaultSampler(it.second.second);

        m_pGlobal->pGPU->CmdBindImageViewSampler(*computePass, it.first, 0, texture->imageId, sampler);
    }

    for (const auto& it : effect.uniformPayloads)
    {
        m_pGlobal->pGPU->CmdBindUniformData(*computePass, it.first, it.second.data(), it.second.size());
    }

    const auto workGroupSize = CalculateWorkGroupSize(*workTexture);
    m_pGlobal->pGPU->CmdDispatch(*computePass, workGroupSize.first, workGroupSize.second, 1);

    m_pGlobal->pGPU->EndComputePass(*computePass);

    //
    // Blit the output back over the input image
    //
    const auto copyPass = m_pGlobal->pGPU->BeginCopyPass(commandBufferId, std::format("BlitEffectResult-{}", effect.userTag));

    m_pGlobal->pGPU->CmdBlitImage(
        *copyPass,
        workTexture->imageId,
        GPU::ImageRegion{
            .layerIndex = 0,
            .mipLevel = 0,
            .offsets = {
                NCommon::Point3DUInt(0,0,0),
                NCommon::Point3DUInt(workTexture->createParams.size.w, workTexture->createParams.size.h, 1)
            }
        },
        inputTexture->imageId,
        GPU::ImageRegion{
            .layerIndex = 0,
            .mipLevel = 0,
            .offsets = {
                NCommon::Point3DUInt(0,0,0),
                NCommon::Point3DUInt(inputTexture->createParams.size.w, inputTexture->createParams.size.h, 1)
            }
        },
        GPU::Filter::Linear,
        false
    );

    m_pGlobal->pGPU->EndCopyPass(*copyPass);
}

std::pair<uint32_t, uint32_t> EffectRenderer::CalculateWorkGroupSize(const LoadedTexture& workTexture)
{
    std::optional<unsigned int> workGroupSizeX;
    std::optional<unsigned int> workGroupSizeY;

    const auto workSize = workTexture.createParams.size; // Should always match render resolution

    // Handle cleanly divisible work group sizes with no fractional part
    if (workSize.w % POST_PROCESS_LOCAL_SIZE_X == 0)
    {
        workGroupSizeX = workSize.w / POST_PROCESS_LOCAL_SIZE_X;
    }
    if (workSize.h % POST_PROCESS_LOCAL_SIZE_Y == 0)
    {
        workGroupSizeY = workSize.h / POST_PROCESS_LOCAL_SIZE_Y;
    }

    // Handle non-cleanly divisible work by rounding up
    if (!workGroupSizeX)
    {
        workGroupSizeX = (unsigned int)((float)workSize.w / (float)POST_PROCESS_LOCAL_SIZE_X) + 1;
    }
    if (!workGroupSizeY)
    {
        workGroupSizeY = (unsigned int)((float)workSize.h / (float)POST_PROCESS_LOCAL_SIZE_Y) + 1;
    }

    return std::make_pair(*workGroupSizeX, *workGroupSizeY);
}

}
