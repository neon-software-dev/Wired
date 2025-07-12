/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Samplers.h"

#include "Global.h"

#include "Wired/GPU/WiredGPU.h"

#include <NEON/Common/Log/ILogger.h>

#include <cassert>
#include <sstream>

namespace Wired::Render
{

std::size_t GetParamsHash(const GPU::SamplerInfo& samplerInfo)
{
    std::stringstream ss;
    ss << (uint32_t)samplerInfo.magFilter << "-";
    ss << (uint32_t)samplerInfo.minFilter << "-";
    ss << (uint32_t)samplerInfo.mipmapMode << "-";
    ss << (uint32_t)samplerInfo.addressModeU << "-";
    ss << (uint32_t)samplerInfo.addressModeV << "-";
    ss << (uint32_t)samplerInfo.addressModeW << "-";
    ss << samplerInfo.anisotropyEnable << "-";
    if (samplerInfo.mipLodBias) { ss << *samplerInfo.mipLodBias << "-"; } else { ss << "-"; }
    if (samplerInfo.minLod) { ss << *samplerInfo.minLod << "-"; } else { ss << "-"; }
    if (samplerInfo.maxLod) { ss << *samplerInfo.maxLod << "-"; } else { ss << "-"; }

    return std::hash<std::string>{}(ss.str());
}

Samplers::Samplers(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

Samplers::~Samplers()
{
    m_pGlobal = nullptr;
}

bool Samplers::StartUp()
{
    m_pGlobal->pLogger->Info("Samplers: Starting Up");

    if (!CreateDefaultSamplers())
    {
        m_pGlobal->pLogger->Fatal("Samplers::StartUp: Failed to create default samplers");
        return false;
    }

    return true;
}

std::optional<GPU::SamplerId> Samplers::GetOrCreateSampler(const GPU::SamplerInfo& samplerInfo, const std::string& userTag)
{
    const auto key = GetParamsHash(samplerInfo);

    const auto it = m_samplers.find(key);
    if (it != m_samplers.cend())
    {
        return it->second;
    }

    m_pGlobal->pLogger->Debug("Samplers: Creating sampler: {} ({})", key, userTag);

    const auto samplerId = m_pGlobal->pGPU->CreateSampler(samplerInfo, userTag);
    if (!samplerId)
    {
        m_pGlobal->pLogger->Fatal("Samplers::GetOrCreateSampler: Failed to create sampler: {}", userTag);
        return std::nullopt;
    }

    m_samplers.insert({key, *samplerId});

    return *samplerId;
}

bool Samplers::CreateDefaultSamplers()
{
    if (!GetOrCreateSampler(GetDefaultSamplerInfo(DefaultSampler::NearestClamp), "NearestClamp")) { return false; }
    if (!GetOrCreateSampler(GetDefaultSamplerInfo(DefaultSampler::NearestRepeat), "NearestRepeat")) { return false; }
    if (!GetOrCreateSampler(GetDefaultSamplerInfo(DefaultSampler::NearestMirrored), "NearestMirrored")) { return false; }
    if (!GetOrCreateSampler(GetDefaultSamplerInfo(DefaultSampler::LinearClamp), "LinearClamp")) { return false; }
    if (!GetOrCreateSampler(GetDefaultSamplerInfo(DefaultSampler::LinearRepeat), "LinearRepeat")) { return false; }
    if (!GetOrCreateSampler(GetDefaultSamplerInfo(DefaultSampler::LinearMirrored), "LinearMirrored")) { return false; }
    if (!GetOrCreateSampler(GetDefaultSamplerInfo(DefaultSampler::AnisotropicClamp), "AnisotropicClamp")) { return false; }
    if (!GetOrCreateSampler(GetDefaultSamplerInfo(DefaultSampler::AnisotropicRepeat), "AnisotropicRepeat")) { return false; }
    if (!GetOrCreateSampler(GetDefaultSamplerInfo(DefaultSampler::AnisotropicMirrored), "AnisotropicMirrored")) { return false; }

    return true;
}

void Samplers::ShutDown()
{
    m_pGlobal->pLogger->Info("Samplers: Shutting Down");

    // Destroy default samplers
    while (!m_samplers.empty())
    {
        const auto it = m_samplers.cbegin();

        m_pGlobal->pGPU->DestroySampler(it->second);

        m_samplers.erase(it);
    }
}

GPU::SamplerId Samplers::GetDefaultSampler(const DefaultSampler& defaultSampler) const
{
    const auto key = GetParamsHash(GetDefaultSamplerInfo(defaultSampler));

    return m_samplers.at(key);
}

GPU::SamplerInfo Samplers::GetDefaultSamplerInfo(const DefaultSampler& sampler)
{
    switch (sampler)
    {
        case DefaultSampler::NearestClamp:
            return GPU::SamplerInfo{
                .magFilter = GPU::SamplerFilter::Nearest,
                .minFilter = GPU::SamplerFilter::Nearest,
                .mipmapMode = GPU::SamplerMipmapMode::Nearest,
                .addressModeU = GPU::SamplerAddressMode::Clamp,
                .addressModeV = GPU::SamplerAddressMode::Clamp,
                .addressModeW = GPU::SamplerAddressMode::Clamp,
                .anisotropyEnable = false
            };
        case DefaultSampler::NearestRepeat:
            return GPU::SamplerInfo{
                .magFilter = GPU::SamplerFilter::Nearest,
                .minFilter = GPU::SamplerFilter::Nearest,
                .mipmapMode = GPU::SamplerMipmapMode::Nearest,
                .addressModeU = GPU::SamplerAddressMode::Repeat,
                .addressModeV = GPU::SamplerAddressMode::Repeat,
                .addressModeW = GPU::SamplerAddressMode::Repeat,
                .anisotropyEnable = false
            };
        case DefaultSampler::NearestMirrored:
            return GPU::SamplerInfo{
                .magFilter = GPU::SamplerFilter::Nearest,
                .minFilter = GPU::SamplerFilter::Nearest,
                .mipmapMode = GPU::SamplerMipmapMode::Nearest,
                .addressModeU = GPU::SamplerAddressMode::Mirrored,
                .addressModeV = GPU::SamplerAddressMode::Mirrored,
                .addressModeW = GPU::SamplerAddressMode::Mirrored,
                .anisotropyEnable = false
            };
        case DefaultSampler::LinearClamp:
            return GPU::SamplerInfo{
                .magFilter = GPU::SamplerFilter::Linear,
                .minFilter = GPU::SamplerFilter::Linear,
                .mipmapMode = GPU::SamplerMipmapMode::Linear,
                .addressModeU = GPU::SamplerAddressMode::Clamp,
                .addressModeV = GPU::SamplerAddressMode::Clamp,
                .addressModeW = GPU::SamplerAddressMode::Clamp,
                .anisotropyEnable = false
            };
        case DefaultSampler::LinearRepeat:
            return GPU::SamplerInfo{
                .magFilter = GPU::SamplerFilter::Linear,
                .minFilter = GPU::SamplerFilter::Linear,
                .mipmapMode = GPU::SamplerMipmapMode::Linear,
                .addressModeU = GPU::SamplerAddressMode::Repeat,
                .addressModeV = GPU::SamplerAddressMode::Repeat,
                .addressModeW = GPU::SamplerAddressMode::Repeat,
                .anisotropyEnable = false
            };
        case DefaultSampler::LinearMirrored:
            return GPU::SamplerInfo{
                .magFilter = GPU::SamplerFilter::Linear,
                .minFilter = GPU::SamplerFilter::Linear,
                .mipmapMode = GPU::SamplerMipmapMode::Linear,
                .addressModeU = GPU::SamplerAddressMode::Mirrored,
                .addressModeV = GPU::SamplerAddressMode::Mirrored,
                .addressModeW = GPU::SamplerAddressMode::Mirrored,
                .anisotropyEnable = false
            };
        case DefaultSampler::AnisotropicClamp:
            return GPU::SamplerInfo{
                .magFilter = GPU::SamplerFilter::Linear,
                .minFilter = GPU::SamplerFilter::Linear,
                .mipmapMode = GPU::SamplerMipmapMode::Linear,
                .addressModeU = GPU::SamplerAddressMode::Clamp,
                .addressModeV = GPU::SamplerAddressMode::Clamp,
                .addressModeW = GPU::SamplerAddressMode::Clamp,
                .anisotropyEnable = true
            };
        case DefaultSampler::AnisotropicRepeat:
            return GPU::SamplerInfo{
                .magFilter = GPU::SamplerFilter::Linear,
                .minFilter = GPU::SamplerFilter::Linear,
                .mipmapMode = GPU::SamplerMipmapMode::Linear,
                .addressModeU = GPU::SamplerAddressMode::Repeat,
                .addressModeV = GPU::SamplerAddressMode::Repeat,
                .addressModeW = GPU::SamplerAddressMode::Repeat,
                .anisotropyEnable = true
            };
        case DefaultSampler::AnisotropicMirrored:
            return GPU::SamplerInfo{
                .magFilter = GPU::SamplerFilter::Linear,
                .minFilter = GPU::SamplerFilter::Linear,
                .mipmapMode = GPU::SamplerMipmapMode::Linear,
                .addressModeU = GPU::SamplerAddressMode::Mirrored,
                .addressModeV = GPU::SamplerAddressMode::Mirrored,
                .addressModeW = GPU::SamplerAddressMode::Mirrored,
                .anisotropyEnable = true
            };
    }

    assert(false);
    return {};
}

}